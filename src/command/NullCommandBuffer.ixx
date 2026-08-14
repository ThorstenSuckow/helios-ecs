/**
 * @file NullCommandBuffer.ixx
 * @brief No-op command buffer implementation used as a default command-buffer type.
 */
module;

export module helios.ecs.command.NullCommandBuffer;

import helios.ecs.command.tags;

export namespace helios::ecs::command {

    /**
     * @brief Command buffer implementation that intentionally performs no work.
     */
    class NullCommandBuffer {

    public:

        using EcsRoleTag = tags::CommandBufferRole;

        template<class T, class... Args>
        void add(Args&&...) {/*intentionally noop*/}

        void flush() noexcept {/*intentionally noop*/}

        void clear() noexcept {/*intentionally noop*/}

        void init() noexcept {/*intentionally noop*/}

    };


}
