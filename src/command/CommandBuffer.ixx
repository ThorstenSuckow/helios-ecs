/**
 * @file CommandBuffer.ixx
 * @brief Type-erased command buffer wrapper using the Concept/Model pattern.
 */
module;

#include <algorithm>
#include <cassert>
#include <memory>
#include <exception>
#include <vector>

export module helios.ecs.command.CommandBuffer;

import helios.ecs.command.concepts;
import helios.ecs.command.types;
import helios.ecs.common.types;

export namespace helios::ecs::command {

    /**
     * @brief Type-erased wrapper for command buffers using the Concept/Model pattern.
     *
     */
    class CommandBuffer {

        using ContextRef = ecs::common::types::ContextRef;
        using ContextTypeId = ecs::common::types::ContextTypeId;
        using CommandBufferTypeId = ecs::command::types::CommandBufferTypeId;

    private:

        /**
         * @brief Internal virtual interface for type-erased dispatch.
         */
        class Concept {
        public:
            virtual ~Concept() = default;
            virtual bool flush(ContextRef& flushContext) noexcept = 0;
            virtual bool clear() noexcept = 0;

            [[nodiscard]] virtual ContextTypeId expectedFlushContextTypeId() const noexcept = 0;

            [[nodiscard]] virtual CommandBufferTypeId typeId() const noexcept = 0;

            [[nodiscard]] virtual void* underlying() noexcept = 0;
            [[nodiscard]] virtual const void* underlying() const noexcept = 0;
        };

        /**
         * @brief Typed model that adapts a concrete buffer to the Concept interface.
         *
         * @tparam TConcreteCommandBuffer The concrete command buffer type.
         */
        template<typename TConcreteCommandBuffer, typename TFlushContextType>
        class Model final : public Concept {

            using FlushContextType = TFlushContextType;

            /**
             * @brief The owned command buffer instance.
             */
            TConcreteCommandBuffer cmdBuffer_;

            ecs::command::types::CommandBufferTypeId typeId_ = ecs::command::types::CommandBufferTypeId::template id<TConcreteCommandBuffer>();


            public:

            explicit Model(TConcreteCommandBuffer cmdBuffer) :  cmdBuffer_(std::move(cmdBuffer)) {}

            bool flush(ContextRef& flushContext) noexcept override {

                if (auto*ctx = flushContext.tryGet<FlushContextType>()) {
                    return cmdBuffer_.flush(*ctx);
                }

                return false;
            }

            bool clear() noexcept override {
                return cmdBuffer_.clear();
            }

            [[nodiscard]] ContextTypeId expectedFlushContextTypeId() const noexcept override {
                return ContextTypeId::template id<FlushContextType>();
            }

            [[nodiscard]] void* underlying() noexcept override {
                return &cmdBuffer_;
            }

            [[nodiscard]] const void* underlying() const noexcept override {
                return &cmdBuffer_;
            }

            [[nodiscard]] CommandBufferTypeId typeId() const noexcept override {
                return typeId_;
            }
        };

        /**
         * @brief Owning pointer to the type-erased command buffer.
         */
        std::unique_ptr<Concept> pimpl_;


        explicit CommandBuffer(std::unique_ptr<Concept> pimpl) :
        pimpl_(std::move(pimpl)){}

    public:


        /**
         * @brief Constructs a CommandBuffer wrapping the given concrete buffer.
         *
         * @tparam TConcreteCommandBuffer The concrete buffer type. Must satisfy IsCommandBufferLike.
         *
         * @param cmdBuffer The buffer instance to wrap. Moved into the wrapper.
         */
        template<typename TConcreteCommandBuffer, typename TFlushContext>
        requires concepts::IsCommandBufferLike<TConcreteCommandBuffer>
        static CommandBuffer make(TConcreteCommandBuffer&& cmdBuffer) {
            return CommandBuffer(
                std::make_unique<Model<
                    std::remove_cvref_t<TConcreteCommandBuffer>,
                    TFlushContext
                >>(std::move(cmdBuffer))
            );
        }

        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;

        CommandBuffer& operator=(CommandBuffer&&) = default;
        CommandBuffer(CommandBuffer&&) noexcept = default;

        /**
         * @brief Executes all queued commands and clears the buffer.
         *
         * @param flushContext The current frame's update context.
         *
         * @pre The CommandBuffer must be initialized (not default-constructed).
         */
        bool flush(ContextRef& flushContext) noexcept {
            assert(pimpl_ && "CommandBuffer not initialized");
            return pimpl_->flush(flushContext);
        }

        /**
         * @brief Discards all queued commands without executing them.
         *
         * @pre The CommandBuffer must be initialized.
         */
        bool clear() noexcept {
            assert(pimpl_ && "CommandBuffer not initialized");
            return pimpl_->clear();
        }


        [[nodiscard]] ContextTypeId expectedFlushContextTypeId() const noexcept {
            assert(pimpl_ && "CommandBuffer not initialized");
            return pimpl_->expectedFlushContextTypeId();
        }

        /**
         * @brief Returns a type-erased pointer to the owned buffer instance.
         *
         * @return Non-null void pointer to the underlying concrete buffer.
         *
         * @pre The CommandBuffer must be initialized.
         */
        [[nodiscard]] void* underlying() noexcept {
            assert(pimpl_ && "CommandBuffer not initialized");
            return pimpl_->underlying();
        }

        /**
         * @copydoc underlying()
         */
        [[nodiscard]] const void* underlying() const noexcept {
            assert(pimpl_ && "CommandBuffer not initialized");
            return pimpl_->underlying();
        }
        
        template<typename TConcreteBuffer>
        [[nodiscard]] TConcreteBuffer* tryGet() noexcept {
            assert(pimpl_ && "CommandBuffer not initialized");
            if (CommandBufferTypeId::template id<TConcreteBuffer>() != pimpl_->typeId()) {
                assert(false && "CommandBuffer does not contain the requested concrete buffer type");
                std::terminate();
            }
            
            return static_cast<TConcreteBuffer*>(pimpl_->underlying());
            
        }

    };


}

