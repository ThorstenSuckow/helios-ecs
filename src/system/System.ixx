/**
 * @file System.ixx
 * @brief Type-erased system wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>
#include <optional>

export module helios.ecs.system.System;

import helios.ecs.command.CommandBuffer;
import helios.ecs.command.NullCommandBuffer;
import helios.ecs.command.concepts;

import helios.ecs.common.types;
import helios.ecs.system.concepts;

export namespace helios::ecs::system {


    /**
     * @brief Type-erased wrapper for game logic processors.
     */
    class System {

        using ContextRef = ecs::common::types::ContextRef;
        using ContextTypeId = ecs::common::types::ContextTypeId;
        using CommandBuffer = ecs::command::CommandBuffer;
        using NullCommandBuffer = ecs::command::NullCommandBuffer;

    private:
        /**
         * @brief Internal virtual interface for type erasure.
         */
        class Concept {
        public:

            virtual ~Concept() = default;
            virtual bool update(ContextRef& contextRef) noexcept = 0;

            [[nodiscard]] virtual CommandBuffer* commandBuffer() noexcept = 0;

            [[nodiscard]] virtual ContextTypeId expectedUpdateContextTypeId() const noexcept = 0;

            [[nodiscard]] virtual void* underlying() noexcept = 0;
            [[nodiscard]] virtual const void* underlying() const noexcept = 0;
        };


        template<typename TConcreteSystem, typename TUpdateContext, typename TCommandBuffer>
        class Model final : public Concept {

            using UpdateContextType = TUpdateContext;

            TConcreteSystem system_;

            /**
             * @brief Wrapped CommandBuffer to make sure init/flush can be called with ContextRef.
             */
            CommandBuffer commandBuffer_;

        public:

            explicit Model(TConcreteSystem&& sys, CommandBuffer&& cmdBuffer) :
            system_(std::move(sys)),
            commandBuffer_(std::move(cmdBuffer)) {}

            bool update(ContextRef& contextRef) noexcept override {

                if constexpr (!std::is_same_v<TCommandBuffer, command::NullCommandBuffer>) {

                    static_assert(requires(TConcreteSystem& system, UpdateContextType& ctx, TCommandBuffer& buffer)
                    {
                        {system.update(ctx, buffer)}->std::same_as<bool>;
                    },
                    "TConcreteSystem must have a member function `bool update(UpdateContextType&, TCommandBuffer&)`"
                    );

                    if (auto* ctx = contextRef.tryGet<UpdateContextType>()) {
                        return system_.update(*ctx, *static_cast<TCommandBuffer*>(commandBuffer_.underlying()));
                    }
                    return false;
                } else {
                    static_assert(requires(TConcreteSystem& system, UpdateContextType& ctx)
                        {
                            {system.update(ctx)}->std::same_as<bool>;
                        },
                        "TConcreteSystem must have a member function `bool update(UpdateContextType&)`"
                        );

                    if (auto* ctx = contextRef.tryGet<UpdateContextType>()) {
                        return system_.update(*ctx);
                    }

                    return false;
                }
            }


            [[nodiscard]] ContextTypeId expectedUpdateContextTypeId() const noexcept override {
                return ContextTypeId::template id<UpdateContextType>();
            }

            [[nodiscard]] CommandBuffer* commandBuffer() noexcept override {
                if constexpr(std::is_same_v<TCommandBuffer, command::NullCommandBuffer>) {
                    return nullptr;
                }

                return &commandBuffer_;
            }

            void* underlying() noexcept override {
                return &system_;
            }

            const void* underlying() const noexcept override {
                return &system_;
            }
        };

        std::unique_ptr<Concept> pimpl_;

        explicit System(std::unique_ptr<Concept>&& pimpl)
            : pimpl_(std::move(pimpl))
        {}

    public:

        template<typename TConcreteSystem, typename TUpdateContext, typename TCommandBufferFactory>
        requires concepts::IsRuntimeSystemLike<std::remove_cvref_t<TConcreteSystem>>
        static System make(TConcreteSystem&& system) {
            using SystemType = std::remove_cvref_t<TConcreteSystem>;

            if constexpr(requires { typename SystemType::CommandTypes;}) {
                auto cmdBuffer = TCommandBufferFactory::make(typename SystemType::CommandTypes{});
                using CommandBufferType = std::remove_cvref_t<decltype(cmdBuffer)>;
                auto erasedCmdBuffer = CommandBuffer::make<CommandBufferType, typename TCommandBufferFactory::FlushContextType>(std::move(cmdBuffer));

                return System{std::make_unique<
                    Model<SystemType, TUpdateContext, CommandBufferType>
                    >(std::move(system), std::move(erasedCmdBuffer))};
            } else {
                auto erasedCmdBuffer = CommandBuffer::make<NullCommandBuffer, typename TCommandBufferFactory::FlushContextType>(
                    std::move(NullCommandBuffer{}));
                return System{std::make_unique<
                    Model<SystemType, TUpdateContext, NullCommandBuffer>
                    >(std::move(system), std::move(erasedCmdBuffer))};
            }

        }

        System() = delete;
        System(const System&) = delete;
        System& operator=(const System&) = delete;

        System& operator=(System&&) = default;
        System(System&&) noexcept = default;

        /**
         * @brief Delegates to the wrapped system's `update()` method.
         *
         * @param updateContext The current frame's update context.
         *
         * @pre System must be initialized (pimpl_ != nullptr).
         */
        bool update(ContextRef& contextRef) noexcept {
            assert(pimpl_ && "System not initialized");
            return pimpl_->update(contextRef);
        }

        [[nodiscard]] ContextTypeId expectedUpdateContextTypeId() const noexcept {
            assert(pimpl_ && "System not initialized");
            return pimpl_->expectedUpdateContextTypeId();
        }

        [[nodiscard]] CommandBuffer* commandBuffer() noexcept {
            assert(pimpl_ && "System not initialized");
            return pimpl_->commandBuffer();
        }

        /**
         * @brief Returns a type-erased pointer to the wrapped system instance.
         *
         * @return Pointer to the underlying concrete system.
         *
         * @pre System must be initialized (pimpl_ != nullptr).
         */
        [[nodiscard]] void* underlying() noexcept {
            assert(pimpl_ && "System not initialized");
            return pimpl_->underlying();
        }

        /**
         * @copydoc underlying()
         */
        [[nodiscard]] const void* underlying() const noexcept {
            assert(pimpl_ && "System not initialized");
            return pimpl_->underlying();
        }

    };


}