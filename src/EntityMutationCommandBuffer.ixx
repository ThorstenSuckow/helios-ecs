/**
 * @file EntityMutationCommandBuffer.ixx
 * @brief Deferred command buffer for structural entity mutations (add/remove components, activate/deactivate).
 */
module;

#include <memory>
#include <vector>
#include <cassert>


#include "helios-ecs-config.h"

export module helios.ecs.EntityMutationCommandBuffer;

import helios.ecs.manager.ManagerRegistry;
import helios.ecs.EntityMutationManager;
import helios.ecs.command.CommandBuffer;
import helios.ecs.types;
import helios.ecs.command.CommandBufferRegistry;
import helios.ecs.command.tags;
import helios.ecs.command.CommandHandlerRegistry;

using namespace helios::ecs;
using namespace helios::ecs::types;
export namespace helios::ecs {

    /**
     * @brief Collects deferred entity-mutation commands and dispatches them in bulk.
     */
    template<typename THandle, typename TFlushContext, typename TInitContext>
    class EntityMutationCommandBuffer {

        /** @brief Registry of lazily created per-command-type `InternalBuffer` instances. */
        command::CommandBufferRegistry<TFlushContext, TInitContext> commandBufferRegistry_{};

        /** @brief Registry used to route submitted commands to their handlers. */
        command::CommandHandlerRegistry& commandHandlerRegistry_;

        /**
         * @brief Per-command-type storage and dispatch unit.
         *
         * Created lazily by `EntityMutationCommandBuffer::bufferFor<TCommand>()`.
         * On `flush()` every buffered command is forwarded to the
         * `CommandHandlerRegistry`, which routes it to the `EntityMutationManager`.
         *
         * @tparam TCommandType ECS command struct to buffer (e.g. `AddComponentCommand<C>`).
         */
        template<typename TCommandType>
        class InternalBuffer {

            /** @brief Registry used to submit commands to their handlers on first flush. */
            command::CommandHandlerRegistry& commandHandlerRegistry_;

   
            /** @brief `true` once the handler for `TCommandType` has been registered. */
            bool handlerRegistered_{false};


            /** @brief Buffered commands pending submission. */
            std::vector<TCommandType> commands_;

            public:
            /** @brief Role tag marking this as a command buffer for the engine registry. */
            using EcsRoleTag = command::tags::CommandBufferRole;

            /** @brief Reserves default capacity for the command vector. */
            explicit InternalBuffer(command::CommandHandlerRegistry& commandHandlerRegistry)
                : commandHandlerRegistry_(commandHandlerRegistry) {
                commands_.reserve(DEFAULT_ENTITY_MUTATION_COMMAND_BUFFER_CAPACITY);
            }

            /**
             * @brief Submits all buffered commands as a batch to the `EntityMutationManager` and clears the buffer.
             *
             * On first call, resolves the `EntityMutationManager` from the `ManagerRegistry`.
             *
             * @note Must not be called from concurrently running tasks.
             *
             * @param flushContext Frame-local flush context (currently unused; kept for interface uniformity).
             */
            void flush(TFlushContext& flushContext) {

                if (commandHandlerRegistry_.has<TCommandType>()) {
                    for (auto& cmd : commands_) {
                        commandHandlerRegistry_.submitBatch<TCommandType>(std::move(cmd));
                    }
                }

                clear();
            }

            /** @brief Discards all buffered commands without dispatching them. */
            void clear() {
                commands_.clear();
            }

            /**
             * @brief Wires this buffer to the handler and manager registries.
             *
             * @param commandHandlerRegistry Registry used to route commands to their handlers.
             * @param managerRegistry        Registry providing the `EntityMutationManager<>`.
             */
            void init(TInitContext& initContext) {
                // intentionalle noop
            }

            /**
             * @brief Enqueues a new command, constructing it in-place from `args`.
             *
             * @tparam TArgs Constructor argument types for `TCommandType`.
             * @param  args  Arguments forwarded to the `TCommandType` constructor.
             */
            template<typename ... TArgs>
            void add(TArgs&&... args) {
                commands_.emplace_back(std::forward<TArgs>(args)...);
            }

        };

        /**
         * @brief Returns (or lazily creates) the `InternalBuffer` for `TCommand`.
         *
         * @tparam TCommand Command type whose buffer is requested.
         * @return Non-owning pointer to the buffer.
         */
        template<typename TCommand>
        auto* bufferFor() {

            auto* model = commandBufferRegistry_.template item<InternalBuffer<TCommand>>();

            if (!model) {
                auto& created   =
                    commandBufferRegistry_.template add<InternalBuffer<TCommand>>(
                        CommandBuffer(InternalBuffer<TCommand>{commandHandlerRegistry_})
                    );
                
                return &created;
            }

            return model;
        }

    public:


        /** @brief Role tag marking this as a command buffer for the engine registry. */
        using EcsRoleTag = command::tags::CommandBufferRole;

        explicit EntityMutationCommandBuffer(command::CommandHandlerRegistry& commandHandlerRegistry)
            : commandHandlerRegistry_(commandHandlerRegistry) {}

        /**
         * @brief Enqueues a command of type `TCommand`, constructing it from `args`.
         *
         * `TCommand::Handle_type` must match `THandle`.
         *
         * @tparam TCommand ECS command type to enqueue.
         * @tparam Args     Constructor argument types.
         * @param  args     Arguments forwarded to the command constructor.
         */
        template<typename TCommand, typename... Args>
        requires std::is_same_v<typename TCommand::Handle_type, THandle>
        void add(Args&&... args) {

            auto* model = bufferFor<TCommand>();

            model->add(std::forward<Args>(args)...);
        }

        /**
         * @brief Returns a raw pointer to the `InternalBuffer` for `TCommand`.
         *
         * @tparam TCommand Command type whose buffer is requested.
         * @return Non-owning pointer to the internal buffer.
         */
        template<typename TCommand>
        auto* bufferForCommand() {
            return bufferFor<TCommand>();
        }

        /**
         * @brief Initialises this buffer and all already-registered internal buffers.
         *
         */
        void init(TInitContext& initContext) {

            for (auto* buffer : commandBufferRegistry_.items()) {
                buffer->init(initContext);
            }
        }

        /**
         * @brief Flushes all internal buffers, dispatching every queued command.
         *
         * @param flushContext
         */
        void flush(TFlushContext& flushContext) {
            for (auto* buffer : commandBufferRegistry_.items()) {
                buffer->flush(flushContext);
            }
        }

        /**
         * @brief Discards all queued commands across every internal buffer without dispatching.
         */
        void clear() {
            for (auto* buffer : commandBufferRegistry_.items()) {
                buffer->clear();
            }
        }

    };

}
