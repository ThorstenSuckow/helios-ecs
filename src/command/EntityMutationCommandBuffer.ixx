/**
 * @file EntityMutationCommandBuffer.ixx
 * @brief Deferred command buffer for structural entity mutations (add/remove components, activate/deactivate).
 */
module;

#include <memory>
#include <vector>


#include "../helios-ecs-config.h"

export module helios.ecs.command.EntityMutationCommandBuffer;

import helios.ecs.command.CommandHandlerRegistry;
import helios.ecs.command.CommandBufferRegistry;
import helios.ecs.manager.ManagerRegistry;

import helios.ecs.common.types;
import helios.ecs.common.concepts;

using namespace helios::ecs;
using namespace helios::ecs::common::types;
export namespace helios::ecs::command {

    /**
     * @brief Collects deferred entity-mutation commands and dispatches them in bulk.
     */
    template<typename THandle>
    class EntityMutationCommandBuffer {

        /** @brief Registry of lazily created per-command-type `InternalBuffer` instances. */
        command::CommandBufferRegistry commandBufferRegistry_{};

        /**
         * @brief Per-command-type storage and dispatch unit.
         *
         * Created lazily by `EntityMutationCommandBuffer::bufferFor<TCommand>()`.
         * On `flush()` every buffered command is forwarded to the
         * `CommandHandlerRegistry`.
         *
         * @tparam TCommandType ECS command struct to buffer (e.g. @c AddComponentCommand).
         */
        template<typename TCommandType>
        class InternalBuffer {


            /** @brief Buffered commands pending submission. */
            std::vector<TCommandType> commands_;

            public:


            /** @brief Reserves default capacity for the command vector. */
            explicit InternalBuffer() {
                commands_.reserve(DEFAULT_ENTITY_MUTATION_COMMAND_BUFFER_CAPACITY);
            }

            /**
             * @brief Submits all buffered commands as a batch to the CommandHandlerRegistry and clears the buffer.
             *
             * @note Must not be called from concurrently running tasks.
             *
             * @param flushContext Frame-local flush context (currently unused; kept for interface uniformity).
             */
            template<typename TFlushContext>
            void flush(TFlushContext& flushContext) {

                auto& managerRegistry = flushContext.managerRegistry();
                auto& commandHandlerRegistry = flushContext.commandHandlerRegistry();
                if (commandHandlerRegistry.template has<TCommandType>()) {
                    for (auto& cmd : commands_) {
                        commandHandlerRegistry.template submitBatch<TCommandType>(std::move(cmd));
                    }
                }

                clear();
            }

            /** @brief Discards all buffered commands without dispatching them. */
            void clear() {
                commands_.clear();
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
                        CommandBuffer(InternalBuffer<TCommand>{})
                    );
                
                return &created;
            }

            return model;
        }

    public:




        EntityMutationCommandBuffer() = default;

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
         * @brief Flushes all internal buffers, dispatching every queued command.
         *
         * @param flushContext Frame-local flush context passed to each internal buffer.
         */
        template<typename TFlushContext>
        requires common::concepts::ProvidesCommandHandlerRegistry<TFlushContext, command::CommandHandlerRegistry> &&
           common::concepts::ProvidesManagerRegistry<TFlushContext, ecs::manager::ManagerRegistry>
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
