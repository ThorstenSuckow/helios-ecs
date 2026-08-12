/**
 * @file CommandBuffer.ixx
 * @brief Type-erased command buffer wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>

export module helios.ecs.command.CommandBuffer;

import helios.ecs.command.concepts;

export namespace helios::ecs::command {

    /**
     * @brief Type-erased wrapper for command buffers using the Concept/Model pattern.
     *
     */
    template<typename TFlushContext, typename TInitContext>
    class CommandBuffer {

    private:

        /**
         * @brief Internal virtual interface for type-erased dispatch.
         */
        class Concept {
        public:
            virtual ~Concept() = default;
            virtual void flush(TFlushContext& flushContext) noexcept = 0;
            virtual void clear() noexcept = 0;
            virtual void init(TInitContext& initContext) noexcept = 0;

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

            /**
             * @brief The owned command buffer instance.
             */
            TConcreteCommandBuffer cmdBuffer_;

            public:

            explicit Model(TConcreteCommandBuffer cmdBuffer) :  cmdBuffer_(std::move(cmdBuffer)) {}

            void flush(TFlushContext& flushContext) noexcept override {
                cmdBuffer_.flush(flushContext);
            }

            void init(TInitContext& initContext) noexcept override {
                cmdBuffer_.init(initContext);
            }

            void clear() noexcept override {
                cmdBuffer_.clear();
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
         * @details Ownership of the buffer is transferred into a heap-allocated
         * Model<TConcreteCommandBuffer>. The concrete type is erased after construction.
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
        void flush(TFlushContext& flushContext) noexcept {
            assert(pimpl_ && "CommandBuffer not initialized");
            pimpl_->flush(flushContext);
        }

        /**
         * @brief Discards all queued commands without executing them.
         *
         * @pre The CommandBuffer must be initialized.
         */
        void clear() noexcept {
            assert(pimpl_ && "CommandBuffer not initialized");
            pimpl_->clear();
        }

        /**
         * @brief Initializes the command buffer with the given registries.
         *
         * @param initContext The initialization context.
         */
        void init(TInitContext& initContext) noexcept {
            assert(pimpl_ && "CommandBuffer not initialized");
            pimpl_->init(initContext);
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

