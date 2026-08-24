/**
 * @file System.ixx
 * @brief Type-erased system wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>
#include <optional>
#include <variant>

export module helios.ecs.system.System;

import helios.core.common.traits;
import helios.core.common.concepts;

import helios.ecs.command.CommandBuffer;
import helios.ecs.command.NullCommandBuffer;
import helios.ecs.command.concepts;
import helios.ecs.command.traits;

import helios.ecs.common;
import helios.ecs.system.concepts;
import helios.ecs.system.types;

export namespace helios::ecs::system {


    /**
     * @brief Type-erased wrapper for game logic processors.
     */
    class System {


        using CommandBuffer = ecs::command::CommandBuffer;
        using NullCommandBuffer = ecs::command::NullCommandBuffer;
        using EcsDataContainer = ecs::common::container::EcsDataContainer;
        using EcsDataContainerArgumentResolver = ecs::common::container::EcsDataContainerArgumentResolver;
    

    private:
        /**
         * @brief Internal virtual interface for type erasure.
         */
        class Concept {
        public:

            virtual ~Concept() = default;
            virtual bool update(EcsDataContainer& ecsDataContainer) noexcept = 0;

            [[nodiscard]] virtual CommandBuffer* commandBuffer() noexcept = 0;

            [[nodiscard]] virtual void* underlying() noexcept = 0;
            [[nodiscard]] virtual const void* underlying() const noexcept = 0;

            virtual bool flush(EcsDataContainer& ecsDataContainer) noexcept = 0;
        };


        template<typename TConcreteSystem>
        class Model final : public Concept {

            TConcreteSystem system_;

            static consteval auto updateFunction() {
                if constexpr(ecs::system::concepts::IsCallableSystem<TConcreteSystem>) {
                    return &TConcreteSystem::operator();
                } else {
                    return &TConcreteSystem::update;
                }
            }

            using UpdateFunction = decltype(updateFunction());
            using Traits = core::common::traits::FunctionSignatureTraits<UpdateFunction>;

            using CommandBufferInfo = ecs::command::traits::CommandBufferFromArguments<typename Traits::ArgumentTypes>;
            static_assert(CommandBufferInfo::Count <= 1, "System update function must have at most one command buffer argument.");
            using ConcreteCommandBufferType = CommandBufferInfo::Type;

            template<std::size_t... Idx>
            auto invokeUpdate(EcsDataContainer& ecsDataContainer,
                ConcreteCommandBufferType& concreteCommandBuffer, std::index_sequence<Idx...>) {

                if constexpr(ecs::system::concepts::IsCallableSystem<TConcreteSystem>) {
                    return std::invoke(system_, EcsDataContainerArgumentResolver::resolve<
                        typename Traits::template Arg<Idx>,
                        ConcreteCommandBufferType
                    >(ecsDataContainer, concreteCommandBuffer)...);
                } else {
                    return system_.update(
                        EcsDataContainerArgumentResolver::resolve<
                            typename Traits::template Arg<Idx>,
                            ConcreteCommandBufferType
                        >(ecsDataContainer, concreteCommandBuffer)...);
                }

            }

            using ProducedFrameResultType = typename Traits::ReturnType;

            using StoredFrameResultType = std::conditional_t<
                std::is_void_v<ProducedFrameResultType>,
                std::monostate,
                ProducedFrameResultType
            >;

            std::optional<StoredFrameResultType> frameResult_;

            CommandBuffer commandBuffer_{ConcreteCommandBufferType{}};

            void updateAndStore(EcsDataContainer& ecsDataContainer) {
                if constexpr(std::is_void_v<ProducedFrameResultType>) {
                    invokeUpdate(ecsDataContainer, *commandBuffer_.tryGet<ConcreteCommandBufferType>(),
                        std::make_index_sequence<Traits::NumArgs>{});
                } else {
                    frameResult_.emplace(invokeUpdate(ecsDataContainer, *commandBuffer_.tryGet<ConcreteCommandBufferType>(),
                        std::make_index_sequence<Traits::NumArgs>{}));
                }

            }

        public:

            explicit Model(TConcreteSystem&& sys) :
            system_(std::move(sys)) {}

            bool update(EcsDataContainer& ecsDataContainer) noexcept override {
                updateAndStore(ecsDataContainer);
                return true;
            }


            [[nodiscard]] CommandBuffer* commandBuffer() noexcept override {
                if constexpr(std::is_same_v<ConcreteCommandBufferType, command::NullCommandBuffer>) {
                    return nullptr;
                }

                return &commandBuffer_;
            }

            bool flush(EcsDataContainer& ecsDataContainer) noexcept override {
                if constexpr(std::is_void_v<ProducedFrameResultType>) {
                    return true;
                } else {
                    ecsDataContainer.emplace<ProducedFrameResultType>(std::move(*frameResult_));
                    frameResult_.reset();
                    return true;
                };
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


        template<typename TConcreteSystem>
        explicit System(TConcreteSystem&& system)
           : pimpl_(std::make_unique<Model<std::remove_cvref_t<TConcreteSystem>>>(std::move(system)))
        {}

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
        bool update(EcsDataContainer& ecsDataContainer) noexcept {
            assert(pimpl_ && "System not initialized");
            return pimpl_->update(ecsDataContainer);
        }

        bool flush(EcsDataContainer& ecsDataContainer) {
            assert(pimpl_ && "System not initialized");
            return pimpl_->flush(ecsDataContainer);
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