/**
 * @file Manager.ixx
 * @brief Type-erased manager wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>
#include <variant>

export module helios.ecs.manager.Manager;

import helios.ecs.entity.traits;

import helios.core.common.traits;

import helios.ecs.common.InvocationContext;
import helios.ecs.common.types;
import helios.ecs.common.container;

import helios.ecs.manager.types;
import helios.ecs.manager.concepts;

import helios.ecs.entity.EntityMutationBuffer;
import helios.ecs.entity.EntityMutationManager;

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
    using EntityMutationBuffer = ecs::entity::EntityMutationBuffer<THandle, TWriteComponents...>;

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

        virtual bool sync(EcsDataContainer& dataContainer) noexcept = 0;
        virtual bool commit(EcsDataContainer& dataContainer) noexcept = 0;
        virtual bool publish(EcsDataContainer& dataContainer) noexcept = 0;
        virtual bool flush(EcsDataContainer& dataContainer) noexcept = 0;

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

        using CommitFunction = decltype(&TConcreteManager::commit);
        using InvocationContext= ecs::common::InvocationContext<CommitFunction>;
        InvocationContext invocationContext_{};
        using ConcreteCommandBufferType = typename InvocationContext::ConcreteCommandBufferType;

        TConcreteManager manager_;
        CommandBuffer commandBuffer_{ConcreteCommandBufferType{}};

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

        bool sync(EcsDataContainer& ecsDataContainer) noexcept override {
            return invocationContext_.syncRequiredStructuralState(ecsDataContainer);
        }

        bool commit(EcsDataContainer& ecsDataContainer) noexcept override {

            EcsDataContainerFunctionInvoker::invoke<&TConcreteManager::commit>(
                manager_,
                ecsDataContainer,
                invocationContext_.entityMutationBuffers(),
                ecsDataContainer,
                *static_cast<ConcreteCommandBufferType*>(commandBuffer_.underlying())
            );

            return true;
        }

        bool publish(EcsDataContainer& ecsDataContainer) noexcept override {
            return invocationContext_.publishEntityMutations(ecsDataContainer);
        }

        bool flush(EcsDataContainer& ecsDataContainer) noexcept override {
            if constexpr (hasCommandBuffer()) {
                return commandBuffer_.flush(ecsDataContainer);
            }
            return true;
        }


        bool init(EcsDataContainer& ecsDataContainer) noexcept override {
            EcsDataContainerFunctionInvoker::invoke<&TConcreteManager::init>(
                manager_, ecsDataContainer,
                invocationContext_.entityMutationBuffers()
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

    bool sync(EcsDataContainer& dataContainer) noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->sync(dataContainer);
    }

    bool commit(EcsDataContainer& dataContainer) noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->commit(dataContainer);
    }

    bool publish(EcsDataContainer& dataContainer) noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->publish(dataContainer);
    }

    bool flush(EcsDataContainer& ecsDataContainer) noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->flush(ecsDataContainer);
    }

    bool execute(EcsDataContainer& ecsDataContainer) noexcept {
        assert(pimpl_ && "Manager not initialized");
        if (!sync(ecsDataContainer)) {
            return false;
        }
        if (!commit(ecsDataContainer)) {
            return false;
        }
        if (!publish(ecsDataContainer)) {
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
