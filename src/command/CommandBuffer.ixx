/**
 * @file CommandBuffer.ixx
 * @brief Type-erased command buffer wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>

export module helios.ecs.command.CommandBuffer;

import helios.ecs.command.concepts;
import helios.ecs.common.types;

export namespace helios::ecs::command {

    /**
     * @brief Type-erased wrapper for command buffers using the Concept/Model pattern.
     *
     */
    class CommandBuffer {

        using ContextRef = ecs::common::types::ContextRef;
        using ContextTypeId = ecs::common::types::ContextTypeId;

    private:

        /**
         * @brief Internal virtual interface for type-erased dispatch.
         */
        class Concept {
        public:
            virtual ~Concept() = default;
            virtual bool flush(ContextRef& flushContext) noexcept = 0;
            virtual bool clear() noexcept = 0;
            virtual bool init(ContextRef& initContext) noexcept = 0;

            [[nodiscard]] virtual ContextTypeId expectedFlushContextTypeId() const noexcept = 0;
            [[nodiscard]] virtual ContextTypeId expectedInitContextTypeId() const noexcept = 0;

            [[nodiscard]] virtual void* underlying() noexcept = 0;
            [[nodiscard]] virtual const void* underlying() const noexcept = 0;
        };

        /**
         * @brief Typed model that adapts a concrete buffer to the Concept interface.
         *
         * @tparam TConcreteCommandBuffer The concrete command buffer type.
         */
        template<typename TConcreteCommandBuffer>
        class Model final : public Concept {

            using FlushContextType = typename TConcreteCommandBuffer::FlushContextType;
            using InitContextType = typename TConcreteCommandBuffer::InitContextType;

            /**
             * @brief The owned command buffer instance.
             */
            TConcreteCommandBuffer cmdBuffer_;



            public:

            explicit Model(TConcreteCommandBuffer cmdBuffer) :  cmdBuffer_(std::move(cmdBuffer)) {}

            bool flush(ContextRef& flushContext) noexcept override {

                if (auto*ctx = flushContext.tryGet<FlushContextType>()) {
                    return cmdBuffer_.flush(*ctx);
                }

                return false;
            }

            bool init(ContextRef& initContext) noexcept override {

                if (auto*ctx = initContext.tryGet<InitContextType>()) {
                    return cmdBuffer_.init(*ctx);
                }

                return false;
            }

            bool clear() noexcept override {
                return cmdBuffer_.clear();
            }

            [[nodiscard]] ContextTypeId expectedFlushContextTypeId() const noexcept override {
                return ContextTypeId::template id<FlushContextType>();
            }

            [[nodiscard]] ContextTypeId expectedInitContextTypeId() const noexcept override {
                return ContextTypeId::template id<InitContextType>();
            }

            [[nodiscard]] void* underlying() noexcept override {
                return &cmdBuffer_;
            }

            [[nodiscard]] const void* underlying() const noexcept override {
                return &cmdBuffer_;
            }
        };

        /**
         * @brief Owning pointer to the type-erased command buffer.
         */
        std::unique_ptr<Concept> pimpl_;

    public:

        CommandBuffer() = default;

        /**
         * @brief Constructs a CommandBuffer wrapping the given concrete buffer.
         *
         * @tparam TConcreteCommandBuffer The concrete buffer type. Must satisfy IsCommandBufferLike.
         *
         * @param cmdBuffer The buffer instance to wrap. Moved into the wrapper.
         */
        template<typename TConcreteCommandBuffer>
        requires concepts::IsCommandBufferLike<TConcreteCommandBuffer>
        explicit CommandBuffer(TConcreteCommandBuffer cmdBuffer) : pimpl_(
            std::make_unique<Model<TConcreteCommandBuffer>>(std::move(cmdBuffer))
        ) {}

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

        /**
         * @brief Initializes the command buffer with the given registries.
         *
         * @param initContext The initialization context.
         */
        bool init(ContextRef& initContext) noexcept {
            assert(pimpl_ && "CommandBuffer not initialized");
            return pimpl_->init(initContext);
        }

        [[nodiscard]] ContextTypeId expectedFlushContextTypeId() const noexcept {
            assert(pimpl_ && "CommandBuffer not initialized");
            return pimpl_->expectedFlushContextTypeId();
        }

        [[nodiscard]] ContextTypeId expectedInitContextTypeId() const noexcept {
            assert(pimpl_ && "CommandBuffer not initialized");
            return pimpl_->expectedInitContextTypeId();
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

    };


}

