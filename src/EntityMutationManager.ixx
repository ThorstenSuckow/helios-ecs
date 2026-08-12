/**
 * @file EntityMutationManager.ixx
 * @brief Applies deferred entity-mutation commands to the ECS world.
 */
module;

#include <vector>
#include <format>
#include "helios-ecs-config.h"
#include <optional>
#include <algorithm>
#include <iterator>
#include <cassert>

export module helios.ecs.EntityMutationManager;

import helios.ecs.common.commands;
import helios.ecs.types;
import helios.ecs.EntityManager;

import helios.core.thread.JobSystem;

import helios.ecs.command.CommandBuffer;
import helios.ecs.command.CommandBufferRegistry;
import helios.ecs.command.tags;
import helios.ecs.command.types;


import helios.ecs.command.CommandHandlerRegistry;
import helios.ecs.manager.ManagerRegistry;
import helios.ecs.manager.tags;
import helios.ecs.manager.types;

import helios.ecs.common.components;


import helios.core.log;

import helios.ecs.concepts;
import helios.core.log;

using namespace helios::ecs::common::components;
using namespace helios::core::thread;
using namespace helios::ecs;
using namespace helios::ecs::types;
using namespace helios::ecs::concepts::traits;
using namespace helios::ecs::common::commands;
using namespace helios::core::log;

#define HELIOS_LOG_SCOPE "helios::ecs::EntityMutationManager"
export namespace helios::ecs {


    /**
     * @brief Receives submitted ECS mutation commands and applies them during flush.
     *
     * Acts as the write-back stage for `EntityMutationCommandBuffer`: commands
     * are submitted directly via `submit()` or `submitBatch()` and buffered per
     * command type. On `flush()` / `flushParallel()` each buffer applies its
     * mutations to the entity manager.
     *
     * @tparam TEntityManager Entity manager type identifying the target ECS registry.
     */
    template<typename THandle, typename TExecutionContext, typename TInitContext>
    class EntityMutationManager {


        /** @brief Reference to the entity manager mutations are applied to. */
        EntityManager<THandle>& entityManager_;

        /** @brief Registry of lazily created per-command-type `InternalExecutionManager` instances. */
        manager::ManagerRegistry<TExecutionContext, TInitContext> internalExecutionManagerRegistry_{};

        /** @brief Job system used by `flushParallel()` for concurrent buffer execution. */
        JobSystem& jobSystem_;

        command::CommandHandlerRegistry& commandHandlerRegistry_;


        /**
         * @brief Maps component type IDs to the `ManagerTypeId`s of their associated buffers.
         *
         * Indexed by component type ID; each slot holds one or more buffer IDs that can be
         * flushed independently in parallel.
         */
        std::vector<std::vector<manager::types::ManagerTypeId<TExecutionContext, TInitContext>>> componentToInternalExecutorGroups_{};

        /** @brief Ordered list of component type IDs that have at least one registered buffer. */
        std::vector<std::size_t> executorGoupIndices_;

        /** @brief Module-scoped logger. */
        static inline auto& logger_ = helios::core::log::LogManager::loggerForScope(HELIOS_LOG_SCOPE);

        /**
         * @brief Per-command-type buffer that applies mutations on flush.
         *
         * Two `flush()` overloads exist, selected by concept constraints:
         * one for component add/remove, one for entity activation/deactivation.
         *
         * @tparam TCommandType ECS command type stored in this buffer.
         */
        template<typename TCommandType>
        class InternalExecutionManager {

            /** @brief Buffered commands pending application. */
            std::vector<TCommandType> commands_;

            /** @brief Reference to the entity manager mutations are applied to. */
            EntityManager<THandle>& entityManager_;

            public:

            using Handle_type = THandle;

            using ExecutionContext_type = TExecutionContext;

            using InitContext_type = TInitContext;

            /** @brief Role tag identifying this as a command buffer in the engine registry. */
            using EcsRoleTag = command::tags::CommandBufferRole;

            /** @brief The command type stored in this buffer. */
            using Command_type = TCommandType;

            /** @brief Component type targeted by the buffered commands. */
            using Component_type = typename TCommandType::Component_type;

            /** @brief Reserves default capacity for the command vector. */
            InternalExecutionManager(EntityManager<THandle>& entityManager) : entityManager_(entityManager) {
                commands_.reserve(DEFAULT_ENTITY_MUTATION_COMMAND_BUFFER_CAPACITY);
            }

            /**
             * @brief Applies all buffered add/remove-component commands directly via the entity manager.
             *
             * Invalid entity handles are silently skipped.
             *
             * @param executionContext Execution context (currently unused; kept for interface uniformity).
             */
            void executeCommands(TExecutionContext& executionContext)
            requires IsAddComponentCommand_v<TCommandType> || IsRemoveComponentCommand_v<TCommandType> {

                using Component_type = typename TCommandType::Component_type;

                logger_.info("Processing {0} commands", commands_.size());

                for (auto& command : commands_) {
                    if (!entityManager_.isValid(command.handle)) {
                        continue;
                    }
                    if constexpr (IsAddComponentCommand_v<TCommandType>) {
                        entityManager_.template emplace<Component_type>(command.handle, std::move(command.component));
                    } else {
                        std::ignore = entityManager_.template remove<Component_type>(command.handle);
                    }
                }

                clear();
            }


            /** @brief Discards all buffered commands without applying them. */
            void clear() {
                commands_.clear();
            }

            /** @brief No-op; satisfies the buffer initialisation interface. */
            void init(TInitContext& initContext) {/* noop*/ }

            /**
             * @brief Stores a command for deferred execution.
             *
             * @param commandType Command instance to buffer.
             */
            void add(TCommandType commandType) {
                commands_.emplace_back(std::forward<TCommandType>(commandType));
            }

            /**
             * @brief Moves all commands from `incoming` into this buffer.
             *
             * If the buffer is empty the vectors are swapped (zero-copy);
             * otherwise `incoming` is appended via move iterators, the cleared.
             *
             *
             * @param incoming Source vector; left in a valid but unspecified state after the call.
             */
            void add(std::vector<TCommandType>& incoming) {

                if (commands_.empty()) {
                     commands_.swap(incoming);
                } else {
                    commands_.reserve(incoming.size() + commands_.size());
                    commands_.insert(
                        commands_.end(),
                        std::make_move_iterator(incoming.begin()),
                        std::make_move_iterator(incoming.end())
                    );
                    incoming.clear();
                }
            }

        };

        /**
         * @brief Returns (or lazily creates) the `InternalExecutionManager` for `TCommand`.
         *
         * @tparam TCommand Command type whose buffer is requested.
         * @return Non-owning pointer to the buffer.
         */
        template<typename TCommand>
        auto* executorFor() {

            auto* executor = internalExecutionManagerRegistry_.template item<InternalExecutionManager<TCommand>>();

            if (!executor) {
                auto& created   =
                    internalExecutionManagerRegistry_.template add<InternalExecutionManager<TCommand>>(
                        Manager(InternalExecutionManager<TCommand>{entityManager_})
                    );

                using Component_type = typename TCommand::Component_type;
                const auto cv = ComponentTypeId<THandle>::template id<Component_type>().value();

                if (componentToInternalExecutorGroups_.size() <= cv) {
                    componentToInternalExecutorGroups_.resize(cv + 1);
                }

                if (componentToInternalExecutorGroups_[cv].empty()) {
                    executorGoupIndices_.push_back(cv);
                }
                componentToInternalExecutorGroups_[cv].push_back(manager::types::ManagerTypeId<TExecutionContext, TInitContext>::template id<InternalExecutionManager<TCommand>>());

                std::ignore = entityManager_.template  ensureSparseSet<Component_type>();

                return &created;
            }

            return executor;
        }

    public:

        /** @brief The entity handle type this manager operates on. */
        using Handle_type = THandle;

        /** @brief Role tag identifying this as a manager in the engine registry. */
        using EcsRoleTag = manager::tags::ManagerRole;

        /**
         * @brief Constructs the manager bound to `entityManager` and `jobSystem`.
         *
         * @param entityManager Entity manager mutations are applied to.
         * @param jobSystem      Job system used for parallel flush execution.
         */
        explicit EntityMutationManager(
            command::CommandHandlerRegistry& commandHandlerRegistry, EntityManager<THandle>& entityManager, JobSystem& jobSystem)
        : commandHandlerRegistry_(commandHandlerRegistry), entityManager_(entityManager), jobSystem_(jobSystem) {}

        /**
         * @brief Accepts a command from the `CommandHandlerRegistry` and enqueues it.
         *
         * `TCommand::Handle_type` must match `THandle`.
         *
         * @tparam TCommand Deduced ECS command type.
         * @param  command  Command instance forwarded into the internal buffer.
         * @return `true` unconditionally (reserved for future error reporting).
         */
        template<typename TCommand>
        requires std::is_same_v<typename TCommand::Handle_type, THandle>
        bool submit(TCommand&& command) {

            using Command_type = std::remove_cvref_t<TCommand>;

            auto* executor = executorFor<Command_type>();
            executor->add(std::forward<TCommand>(command));

            return true;
        }

        /**
         * @brief Accepts a batch of commands and enqueues them all at once.
         *
         * Delegates to `InternalExecutionManager::add(vector)` which swaps or appends
         * depending on whether the buffer is currently empty. The origin vector is
         * cleared afterwards.
         *
         * @tparam TCommand Deduced ECS command type. `TCommand::Handle_type` must match `THandle`.
         * @param  commands Source vector forwarded to the internal buffer.
         * @return `true` unconditionally (reserved for future error reporting).
         */
        template<typename TCommand>
        requires std::is_same_v<typename TCommand::Handle_type, THandle>
        bool submitBatch(std::vector<TCommand>& commands) {

            auto* executor = executorFor<TCommand>();
            executor->add(commands);

            return true;
        }

        /**
         * @brief Initialises the manager. Currently a no-op.
         *
         * @param commandHandlerRegistry Unused; kept for interface uniformity.
         */
        void init(TInitContext& initContext) {

            commandHandlerRegistry_.registerHandlerForCommandGroup<
                command::types::CommandGroup<AddComponentCommand, THandle>
            >(*this);

            commandHandlerRegistry_.registerHandlerForCommandGroup<
                command::types::CommandGroup<RemoveComponentCommand, THandle>
            >(*this);

        }

        /**
         * @brief Flushes all internal buffers sequentially, applying every queued mutation.
         *
         * @param updateContext Frame-local ECS context forwarded to each buffer's flush.
         */
        void executeCommands(TExecutionContext& executionContext) {
            for (auto* manager : internalExecutionManagerRegistry_.items()) {
                manager->executeCommands(executionContext);
            }

            entityManager_.finalizeMutations();
        }

        /**
         * @brief Flushes independent buffer groups concurrently via the `JobSystem`.
         *
         * Each component-type group is dispatched as a separate job; groups that operate
         * on different component types are assumed to be executable in parallel without contention.
         *
         * @param executionContext Execution context forwarded to each buffer's flush.
         */
        void flushParallel(TExecutionContext& executionContext) {

            std::vector<std::size_t> activeIndices;
            for (const auto idx : executorGoupIndices_) {
                if (!componentToInternalExecutorGroups_[idx].empty()) {
                    activeIndices.push_back(idx);
                }
            }
            jobSystem_.runAndWait(activeIndices.size(), [&](const std::size_t groupIndex) {
                for (const auto executorTypeId  : componentToInternalExecutorGroups_[activeIndices[groupIndex]]) {
                    logger_.info("Processing MutationCommandBuffer {0}", executorTypeId.value());
                    auto* executor = internalExecutionManagerRegistry_.item(executorTypeId);
                    executor->executeCommands(executionContext);
                }
                entityManager_.finalizeMutations(ComponentTypeId<THandle>{activeIndices[groupIndex]});
            });


        }


    };


}