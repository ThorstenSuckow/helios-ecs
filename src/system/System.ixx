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
import helios.core.common.types;

import helios.ecs.component.components;

import helios.ecs.entity.EntityManager;
import helios.ecs.entity.mutation.EntityMutationBuffer;
import helios.ecs.entity.mutation.traits;
import helios.ecs.entity.query.NullQuery;

import helios.ecs.command.CommandBuffer;

import helios.ecs.command.NullCommandBuffer;
import helios.ecs.command.concepts;
import helios.ecs.command.traits;

import helios.ecs.common;
import helios.ecs.system.concepts;
import helios.ecs.system.types;

import helios.ecs.common.InvocationContext;

export namespace helios::ecs::system {

/**
 * @brief Type-erased wrapper for game logic processors.
 */
class System {

    using CommandBuffer = ecs::command::CommandBuffer;
    using NullCommandBuffer = ecs::command::NullCommandBuffer;
    using EcsDataContainer = ecs::common::container::EcsDataContainer;

    template<typename TMutationBufferTuple>
    using EcsDataContainerArgumentResolver = ecs::common::container::EcsDataContainerArgumentResolver<TMutationBufferTuple>;

    template <typename THandle, typename ... TWriteComponents>
    using EntityMutationBuffer = ecs::entity::mutation::EntityMutationBuffer<THandle, TWriteComponents...>;

private:
    /**
     * @brief Internal virtual interface for type erasure.
     */
    class Concept {
    public:
        virtual ~Concept() = default;

        virtual bool commitMutations(EcsDataContainer& ecsDataContainer) noexcept = 0;
        virtual bool update(EcsDataContainer& ecsDataContainer) noexcept = 0;
        virtual bool flush(EcsDataContainer& ecsDataContainer) noexcept = 0;

        [[nodiscard]] virtual void* underlying() noexcept = 0;
        [[nodiscard]] virtual const void* underlying() const noexcept = 0;

    };

    template <typename TConcreteSystem>
    class Model final : public Concept {
        TConcreteSystem system_;

        static consteval auto updateFunction() {
            if constexpr (ecs::system::concepts::IsCallableSystem<TConcreteSystem>) {
                return &TConcreteSystem::operator();
            } else {
                return &TConcreteSystem::update;
            }
        }

        /* Invocation Context */
        using UpdateFunction = decltype(updateFunction());
        using InvocationContext = ecs::common::InvocationContext<UpdateFunction>;
        using EntityMutationBufferTypes = typename InvocationContext::EntityMutationBufferTypes;
        using ConcreteCommandBufferType = typename InvocationContext::ConcreteCommandBufferType;
        using UpdateFunctionTraits = InvocationContext::InvocationFunctionTraits;
        template <std::size_t TIdx>
        using UpdateFuncArgType = typename InvocationContext::template InvocationFunctionArgType<TIdx>;
        using ProducedSystemResultType = UpdateFunctionTraits::ReturnType;

        using StoredSystemResultType =
        std::conditional_t<std::is_void_v<ProducedSystemResultType>, std::monostate, ProducedSystemResultType>;

        std::optional<StoredSystemResultType> systemResult_;

        CommandBuffer commandBuffer_{ConcreteCommandBufferType{}};

        EntityMutationBufferTypes entityMutationBuffers_{};

        template <std::size_t... Idx>
        auto invokeUpdate(
            EcsDataContainer& ecsDataContainer,
            ConcreteCommandBufferType& concreteCommandBuffer,
            std::index_sequence<Idx...>
        ) {
            auto resolver =  EcsDataContainerArgumentResolver(ecsDataContainer, entityMutationBuffers_);

            if constexpr (ecs::system::concepts::IsCallableSystem<TConcreteSystem>) {
                return std::invoke(
                    system_,
                    resolver.template resolve<Idx,
                    UpdateFuncArgType<Idx>>(concreteCommandBuffer)...
                );
            } else {
                return system_.update(
                    resolver.template resolve<Idx,
                    UpdateFuncArgType<Idx>>(concreteCommandBuffer)...
                );
            }
        }

        static constexpr bool hasCommandBuffer() noexcept {
            return !std::is_same_v<ConcreteCommandBufferType, command::NullCommandBuffer>;
        }

        void updateAndStore(EcsDataContainer& ecsDataContainer) {
            if constexpr (std::is_void_v<ProducedSystemResultType>) {
                invokeUpdate(
                    ecsDataContainer,
                    *commandBuffer_.tryGet<ConcreteCommandBufferType>(),
                    std::make_index_sequence<UpdateFunctionTraits::NumArgs>{}
                );
            } else {
                systemResult_.emplace(invokeUpdate(
                    ecsDataContainer,
                    *commandBuffer_.tryGet<ConcreteCommandBufferType>(),
                    std::make_index_sequence<UpdateFunctionTraits::NumArgs>{}
                ));
            }
        }



    public:
        explicit Model(TConcreteSystem&& sys) : system_(std::move(sys)) {}

        bool commitMutations(EcsDataContainer& ecsDataContainer) noexcept override {
            constexpr std::size_t BufferCount = std::tuple_size_v<decltype(entityMutationBuffers_)>;
            ([&]<std::size_t... Idx>(std::index_sequence<Idx...>) {

                ([&]() {
                    auto& buffer = std::get<Idx>(entityMutationBuffers_);
                    using BufferType = std::remove_cvref_t<decltype(buffer)>;

                    if constexpr (!std::same_as<BufferType, std::monostate>) {
                        auto& entityManager = ecsDataContainer.get<
                            ecs::entity::EntityManager<typename BufferType::HandleType>
                        >();
                        buffer.flush(entityManager);
                    }
                }(), ...);

            }(std::make_index_sequence<BufferCount>{}));

            return true;
        }

        bool update(EcsDataContainer& ecsDataContainer) noexcept override {
            updateAndStore(ecsDataContainer);
            return true;
        }

        bool flush(EcsDataContainer& ecsDataContainer) noexcept override {
            if constexpr (!std::is_void_v<ProducedSystemResultType>) {
                ecsDataContainer.emplace<ProducedSystemResultType>(std::move(*systemResult_));
                systemResult_.reset();
            }

            if constexpr (hasCommandBuffer()) {
                commandBuffer_.flush(ecsDataContainer);
            }

            return true;
        }

        void* underlying() noexcept override {
            return &system_;
        }

        [[nodiscard]] const void* underlying() const noexcept override {
            return &system_;
        }
    };

    std::unique_ptr<Concept> pimpl_;

    explicit System(std::unique_ptr<Concept>&& pimpl) : pimpl_(std::move(pimpl)) {}

public:
    template <typename TConcreteSystem>
    requires concepts::IsCallableSystem <TConcreteSystem>|| concepts::IsTypedSystem<TConcreteSystem>
    explicit System(TConcreteSystem&& system)
        : pimpl_(std::make_unique<Model<std::remove_cvref_t<TConcreteSystem>>>(std::forward<TConcreteSystem>(system))) {}

    System() = delete;
    System(const System&) = delete;
    System& operator=(const System&) = delete;

    System& operator=(System&&) = default;
    System(System&&) noexcept = default;

    bool update(EcsDataContainer& ecsDataContainer) noexcept {
        assert(pimpl_ && "System not initialized");
        if (!pimpl_->update(ecsDataContainer)) {
            return false;
        }
        if (!pimpl_->commitMutations(ecsDataContainer)) {
            return false;
        }
        return true;
    }

    bool flush(EcsDataContainer& ecsDataContainer) noexcept {
        assert(pimpl_ && "System not initialized");
        return pimpl_->flush(ecsDataContainer);
    }

    [[nodiscard]] const void* underlying() const noexcept {
        assert(pimpl_ && "System not initialized");
        return pimpl_->underlying();
    }

    [[nodiscard]] void* underlying() noexcept {
        assert(pimpl_ && "System not initialized");
        return pimpl_->underlying();
    }
};

} // namespace helios::ecs::system