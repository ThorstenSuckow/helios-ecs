/**
 * @file EntityMutationManager.ixx
 * @brief Applies deferred entity-mutation commands to the ECS world.
 */
module;

#include <algorithm>
#include <cassert>
#include <vector>
#include <exception>
#include <optional>

export module helios.ecs.entity.EntityMutationManager;

import :MutationCommandQueue;

import helios.ecs.common.types;
import helios.ecs.common.container;
import helios.ecs.entity.EntityManager;

import helios.core.thread.JobSystem;

import helios.ecs.command.types;
import helios.ecs.command.commands;

import helios.core.common.container;
import helios.core.common.types;

import helios.ecs.manager.types;

import helios.ecs.component.components;

import helios.core.log;

import helios.ecs.common.concepts;
import helios.ecs.common.types;

import helios.ecs.entity.storage.SparseSet;

using namespace helios::ecs::components;
using namespace helios::core::thread;
using namespace helios::ecs;
using namespace helios::ecs::common::types;
using namespace helios::ecs::common::concepts::traits;
using namespace helios::ecs::command;
using namespace helios::core::log;

#define HELIOS_LOG_SCOPE "helios::ecs::entity::EntityMutationManager"
export namespace helios::ecs::entity {

/**
 * @brief Receives submitted ECS mutation commands and applies them during flush.
 *
 * Acts as the write-back stage for `EntityMutationCommandBuffer`: commands
 * are submitted directly via `submit()` or `submitBatch()` and buffered per
 * command type. On `executeCommands()` each queue applies its
 * mutations to the entity manager.
 *
 * @tparam THandle Entity handle type identifying the target ECS registry.
 */
template <typename THandle>
class EntityMutationManager {

    struct QueueTag{};
    using QueueTypeId = core::common::types::TypeId<QueueTag>;

    using EcsDataContainer = ecs::common::container::EcsDataContainer;

    using EntityManager = ecs::entity::EntityManager<THandle>;

    template<typename TComponent>
    using SparseSet = ecs::entity::storage::SparseSet<TComponent>;

    template<typename TComponent>
    using AddComponentCommand = ecs::commands::AddComponentCommand<TComponent>;
    template<typename TComponent>
    using RemoveComponentCommand = ecs::commands::RemoveComponentCommand<TComponent>;

    using CommandQueue = MutationCommandQueue<THandle>;

    EntityManager* entityManager_{nullptr};
    std::vector<std::optional<CommandQueue>> queueRegistry_;
    std::vector<core::common::types::TypeId_t> registeredQueueIndices_;

    /**
     * @brief Module-scoped logger.
     */
    static inline auto& logger_ = helios::core::log::LogManager::loggerForScope(HELIOS_LOG_SCOPE);


    /**
     * @brief Returns (or lazily creates) the `InternalExecutionManager` for `TCommand`.
     *
     * @tparam TCommand Command type whose buffer is requested.
     * @return Non-owning pointer to the buffer.
     */
    template <typename TCommand>
    auto& queueFor(EntityManager* entityManager) {

        using ComponentType = TCommand::ComponentType;

        const auto queueTypeId = QueueTypeId::template id<ComponentType>();
        const auto idx = queueTypeId.value();
        if (queueRegistry_.size() <= idx) {
            queueRegistry_.resize(idx + 1);
        }


        if (!queueRegistry_[idx]) {

            // ensure SparseSet exists to prevent race conditions during emplace() (AddComponent)
            auto* sparseSet = entityManager->template ensureSparseSet<ComponentType>();

            if (!sparseSet) [[unlikely]] {
                assert(false && "Failed to ensure SparseSet for component type.");
                std::terminate();
            }

            registeredQueueIndices_.push_back(idx);
            queueRegistry_[idx] = CommandQueue::template make<ComponentType>(sparseSet);
        }

        return *queueRegistry_[idx];
    }

public:

    /**
     * @brief The entity handle type this manager operates on.
     */
    using HandleType = THandle;


    template <typename TCommand>
    requires std::is_same_v<typename TCommand::HandleType, THandle>
    bool submit(TCommand&& command) {

        using CommandType = std::remove_cvref_t<TCommand>;

        auto& queue = queueFor<CommandType>(entityManager_);
        queue.add(std::forward<TCommand>(command));

        return true;
    }


    template <typename TCommand>
    requires std::is_same_v<typename TCommand::HandleType, THandle>
    bool submitBatch(std::vector<TCommand>&& commands) {

        auto& queue = queueFor<TCommand>(entityManager_);
        queue.add(std::move(commands));

        return true;
    }

    bool init(EntityManager& entityManager) noexcept {

        assert(entityManager_ == nullptr);
        entityManager_ = &entityManager;

        return true;
    }

    /**
     * @brief Executes all internal queues sequentially, applying every queued mutation.
     *
     * @param entityManager Entity manager to which the mutations are applied.
     */
    bool executeCommands(const EntityManager& entityManager) noexcept {

        for (auto idx : registeredQueueIndices_) {
            queueRegistry_[idx]->executeCommands(entityManager);
        }

        return true;
    }



    void reset() { /* intentionally noop */ }
};

} // namespace helios::ecs::manager