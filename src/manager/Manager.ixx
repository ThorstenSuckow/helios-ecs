/**
 * @file Manager.ixx
 * @brief Type-erased manager wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>
#include <variant>
#include <optional>

export module helios.ecs.manager.Manager;

import helios.ecs.entity.mutation.traits;

import helios.core.common.traits;

import helios.ecs.common.InvocationContext;
import helios.ecs.common.types;
import helios.ecs.common.container;
import helios.ecs.common.RuntimeResultRegistry;
import helios.ecs.common.RuntimeResult;

import helios.ecs.manager.types;
import helios.ecs.manager.concepts;

import helios.ecs.entity.mutation.EntityMutationBuffer;
import helios.ecs.entity.EntityManager;

import helios.ecs.command.concepts;
import helios.ecs.command.traits;

import helios.ecs.command.CommandBuffer;
import helios.ecs.command.NullCommandBuffer;

export namespace helios::ecs::manager {

/**
 * @brief Type-erased wrapper for game world managers.
 */
class Manager {

    using CommandBuffer = ecs::command::CommandBuffer;
    using NullCommandBuffer = ecs::command::NullCommandBuffer;
    using EcsDataContainer = ecs::common::container::EcsDataContainer;

    template <typename THandle, typename ... TWriteComponents>
    using EntityMutationBuffer = ecs::entity::mutation::EntityMutationBuffer<THandle, TWriteComponents...>;

    template<typename TMutationSink>
    using EcsDataContainerArgumentResolver = ecs::common::container::EcsDataContainerArgumentResolver<TMutationSink>;

    using EcsDataContainerFunctionInvoker = ecs::common::container::EcsDataContainerFunctionInvoker;

private:
    /**
     * @brief Internal virtual interface for type erasure.
     */
    class Concept {
    public:
        virtual ~Concept() = default;

        virtual bool init(EcsDataContainer& dataContainer) noexcept = 0;

        virtual void reset() noexcept = 0;

        virtual bool commitMutations(EcsDataContainer& ecsDataContainer) noexcept = 0;
        virtual bool execute(EcsDataContainer& dataContainer) noexcept = 0;
        virtual bool flush(EcsDataContainer& dataContainer) noexcept = 0;
        virtual bool publishResults(EcsDataContainer& ecsDataContainer) noexcept = 0;
        [[nodiscard]] virtual bool hasResult() const noexcept = 0;

        virtual command::CommandBuffer* commandBuffer() noexcept = 0;

        [[nodiscard]] virtual void* underlying() noexcept = 0;
        [[nodiscard]] virtual const void* underlying() const noexcept = 0;
    };

    /**
     * @brief Typed wrapper that adapts a concrete manager to the Concept interface.
     *
     * @tparam TConcreteManager The concrete manager type, must satisfy `IsManagerLike<TConcreteManager>`.
     */
    template <typename TConcreteManager>
    class Model final : public Concept {

        using CommitFunction = decltype(&TConcreteManager::execute);
        using InvocationContext= ecs::common::InvocationContext<CommitFunction>;
        using EntityMutationBufferTypes = typename InvocationContext::EntityMutationBufferTypes;
        using ConcreteCommandBufferType = typename InvocationContext::ConcreteCommandBufferType;
        using ProducedRuntimeResultType = InvocationContext::InvocationFunctionTraits::ReturnType;

        using StoredRuntimeResultType =
        std::conditional_t<std::is_void_v<ProducedRuntimeResultType>, std::monostate, ProducedRuntimeResultType>;


        std::optional<StoredRuntimeResultType> runtimeResult_;


        TConcreteManager manager_;
        CommandBuffer commandBuffer_{ConcreteCommandBufferType{}};
        EntityMutationBufferTypes entityMutationBuffers_{};

        static bool constexpr hasCommandBuffer() noexcept {
            return !std::same_as<NullCommandBuffer, ConcreteCommandBufferType>;
        }

    public:
        explicit Model(TConcreteManager&& manager)
            : manager_(std::move(manager)), commandBuffer_(CommandBuffer{ConcreteCommandBufferType{}}) {}

        [[nodiscard]] command::CommandBuffer* commandBuffer() noexcept override {
            if (!hasCommandBuffer()) {
                return nullptr;
            }

            return &commandBuffer_;
        }

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

        bool execute(EcsDataContainer& ecsDataContainer) noexcept override {

            auto invoke= [&]()->decltype(auto) {
                return EcsDataContainerFunctionInvoker::invoke<&TConcreteManager::execute>(
                    manager_,
                    ecsDataContainer,
                    entityMutationBuffers_,
                    ecsDataContainer,
                    *static_cast<ConcreteCommandBufferType*>(commandBuffer_.underlying())
                );
            };

            if constexpr (!std::is_void_v<ProducedRuntimeResultType>) {
                runtimeResult_.emplace(invoke());
            } else {
                invoke();
            }

            return true;
        }


        bool flush(EcsDataContainer& ecsDataContainer) noexcept override {
            if constexpr (hasCommandBuffer()) {
                return commandBuffer_.flush(ecsDataContainer);
            }
            return true;
        }

        bool publishResults(EcsDataContainer &ecsDataContainer) noexcept override {
            if constexpr (!std::is_void_v<ProducedRuntimeResultType>) {
                auto* resultRegistry = ecsDataContainer.tryGet<ecs::common::RuntimeResultRegistry>();
                assert(resultRegistry && "RuntimeResultRegistry not found in EcsDataContainer");
                resultRegistry->replace<ProducedRuntimeResultType>(
                    common::RuntimeResult(std::move(*runtimeResult_))
                );
                runtimeResult_.reset();
            }
            return true;
        }

        bool hasResult() const noexcept override {
            if constexpr (!std::is_void_v<ProducedRuntimeResultType>) {
                return true;
            }
            return false;
        }

        bool init(EcsDataContainer& ecsDataContainer) noexcept override {
            EcsDataContainerFunctionInvoker::invoke<&TConcreteManager::init>(
                manager_, ecsDataContainer,
                entityMutationBuffers_
            );

            return true;
        }

        void reset() noexcept override {
            manager_.reset();
        }

        void* underlying() noexcept override {
            return &manager_;
        }

        [[nodiscard]] const void* underlying() const noexcept override {
            return &manager_;
        }
    };

    std::unique_ptr<Concept> pimpl_;

public:
    /**
     * @brief Default constructor creating an empty Manager.
     */
    Manager() = delete;

    /**
     * @brief Wraps a concrete manager in a type-erased Manager.
     *
     * @tparam TConcreteManager The concrete manager type, must satisfy `IsManagerLike<TConcreteManager>`.
     *
     * @param manager The concrete manager instance to wrap (moved into internal storage).
     */
    template <typename TConcreteManager>
        requires concepts::IsManagerLike<TConcreteManager>
    explicit Manager(TConcreteManager&& manager)
        : pimpl_(std::make_unique<Model<std::remove_cvref_t<TConcreteManager>>>(std::forward<TConcreteManager>(manager))) {}

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    Manager& operator=(Manager&&) = default;
    Manager(Manager&&) noexcept = default;



    bool init(EcsDataContainer& dataContainer) noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->init(dataContainer);
    }

    bool flush(EcsDataContainer& ecsDataContainer) noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->flush(ecsDataContainer);
    }

    [[nodiscard]] bool hasResult() const noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->hasResult();
    }

    bool execute(EcsDataContainer& ecsDataContainer) noexcept {
        assert(pimpl_ && "Manager not initialized");
        if (!pimpl_->execute(ecsDataContainer)) {
            return false;
        }
        if (!pimpl_-> commitMutations(ecsDataContainer)) {
            return false;
        }
        if (!pimpl_->publishResults(ecsDataContainer)) {
            return false;
        }
        return true;
    }

    void reset() noexcept {
        assert(pimpl_ && "Manager not initialized");
        (*pimpl_).reset();
    }

    [[nodiscard]] void* underlying() noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->underlying();
    }

    [[nodiscard]] const void* underlying() const noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->underlying();
    }
};

} // namespace helios::ecs::manager
