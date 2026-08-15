/**
 * @file TypedCommandBuffer.ixx
 * @brief Compile-time typed command buffer with handler routing.
 */
module;

#include <tuple>
#include <utility>
#include <vector>

export module helios.ecs.command.TypedCommandBuffer;


import helios.ecs.command.CommandHandlerRegistry;
import helios.ecs.command.tags;
import helios.ecs.common.concepts;
import helios.ecs.manager.ManagerRegistry;


export namespace helios::ecs::command {


    /**
     * @brief Compile-time typed command buffer with per-type queues and handler routing.
     *
     * @tparam TCommandTypes The command types this buffer manages.

     */
    template <typename TInitContext, typename TFlushContext, typename... TCommandTypes>
    requires common::concepts::ProvidesCommandHandlerRegistry<TFlushContext, command::CommandHandlerRegistry> &&
        common::concepts::ProvidesCommandHandlerRegistry<TFlushContext, command::CommandHandlerRegistry> &&
        common::concepts::ProvidesManagerRegistry<TFlushContext, ecs::manager::ManagerRegistry>
    class TypedCommandBuffer {


        /**
         * @brief Per-type command queues stored as a tuple of vectors.
         */
        std::tuple<std::vector<TCommandTypes>...> commandQueues_;

        /**
         * @brief Returns the queue for a specific command type.
         *
         * @tparam TCommandType The command type.
         *
         * @return Reference to the command queue.
         */
        template<typename TCommandType>
        std::vector<TCommandType>& commandQueue() noexcept {
            return std::get<std::vector<TCommandType>>(commandQueues_);
        }


        /**
         * @brief Flushes a single command queue.
         *
         *
         * @tparam TCommandType The command type to flush.
         *
         * @param updateContext The current frame's update context.
         */
        template<typename TCommandType>
        void flushCommandQueue(TFlushContext& flushContext) noexcept {

            auto& commandHandlerRegistry = flushContext.commandHandlerRegistry();
            auto& managerRegistry = flushContext.managerRegistry();
            auto& queue = commandQueue<TCommandType>();

            if (queue.empty()) {
                return;
            }

            if (commandHandlerRegistry.template has<TCommandType>()) {
                for (auto& cmd : queue) {
                    commandHandlerRegistry.template submit<TCommandType>(std::move(cmd), managerRegistry);
                }
            }

            queue.clear();
        }

    public:

        using EcsRoleTag = tags::CommandBufferRole;


        /**
         * @brief Enqueues a command of the specified type.
         *
         * @tparam TCommandType The command type. Must be one of the TCommandTypes.
         * @tparam Args Constructor argument types.
         *
         * @param args Arguments forwarded to the command constructor.
         *
         * @note Commands are consumed during `flush(...)` when dispatched to
         * handlers, i.e. they are forwarded as rvalues.
         */
        template<typename TCommandType, typename... Args>
        void add(Args&&... args) {
            auto& queue = commandQueue<TCommandType>();
            queue.emplace_back(std::forward<Args>(args)...);
        }

        /**
         * @brief Binds external services required for command dispatch.
         *
         * @param commandHandlerRegistry Registry used for handler-based command routing.
         */
        void init(TInitContext& initContext) noexcept {
          // intentionally left empoty
        }

        /**
         * @brief Discards all queued commands without executing them.
         */
        void clear() noexcept {
            std::apply([](auto&... queue) { (queue.clear(), ...); }, commandQueues_);
        }

        /**
         * @brief Flushes all command queues in template parameter order.
         */
        void flush(TFlushContext& flushContext) noexcept {
            (flushCommandQueue<TCommandTypes>(flushContext), ...);
        }


    };


}