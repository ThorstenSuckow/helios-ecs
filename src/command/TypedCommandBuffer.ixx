/**
 * @file TypedCommandBuffer.ixx
 * @brief Compile-time typed command buffer with handler routing.
 */
module;

#include <tuple>
#include <utility>
#include <vector>

export module helios.ecs.command.TypedCommandBuffer;

import helios.ecs.common.types;
import helios.ecs.command.CommandHandlerRegistry;

import helios.ecs.common.concepts;
import helios.ecs.manager.ManagerRegistry;

export namespace helios::ecs::command {

/**
 * @brief Compile-time typed command buffer with per-type queues and handler routing.
 *
 * @tparam TCommandTypes The command types this buffer manages.

 */
template <typename... TCommandTypes>
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
    template <typename TCommandType>
    std::vector<TCommandType>& commandQueue() noexcept {
        return std::get<std::vector<TCommandType>>(commandQueues_);
    }

    /**
     * @brief Flushes a single command queue.
     *
     *
     * @tparam TCommandType The command type to flush.
     *
     * @param commandHandlerRegistry The registry used to resolve command handlers.
     */
    template <typename TCommandType>
    void flushCommandQueue(const CommandHandlerRegistry& commandHandlerRegistry) noexcept {

        auto& queue = commandQueue<TCommandType>();

        if (queue.empty()) {
            return;
        }

        if (commandHandlerRegistry.template has<TCommandType>()) {
            for (auto& cmd : queue) {
                commandHandlerRegistry.template submit<TCommandType>(std::move(cmd));
            }
        }

        queue.clear();
    }

public:
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
    template <typename TCommandType, typename... Args>
    void add(Args&&... args) {
        auto& queue = commandQueue<TCommandType>();
        queue.emplace_back(std::forward<Args>(args)...);
    }

    /**
     * @brief Discards all queued commands without executing them.
     */
    bool clear() noexcept {
        std::apply([](auto&... queue) { (queue.clear(), ...); }, commandQueues_);
        return true;
    }

    /**
     * @brief Flushes all command queues in template parameter order.
     */
    bool flush(const CommandHandlerRegistry& commandHandlerRegistry) noexcept {
        (flushCommandQueue<TCommandTypes>(commandHandlerRegistry), ...);
        return true;
    }
};

} // namespace helios::ecs::command