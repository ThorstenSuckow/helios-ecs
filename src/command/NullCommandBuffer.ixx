/**
 * @file NullCommandBuffer.ixx
 * @brief No-op command buffer implementation used as a default command-buffer type.
 */
module;

export module helios.ecs.command.NullCommandBuffer;

import helios.ecs.common.types;

export namespace helios::ecs::command {

/**
 * @brief Command buffer implementation that intentionally performs no work.
 */
class NullCommandBuffer {

public:
    template <class T, class... Args>
    void add(Args&&... /*unused*/) { /*intentionally noop*/ }

    bool flush() noexcept {
        return true;
    }

    bool clear() noexcept {
        return true;
    }
};

} // namespace helios::ecs::command
