/**
 * @file NullCommandBuffer.ixx
 * @brief No-op command buffer implementation used as a default command-buffer type.
 */
module;

export module helios.ecs.command.NullCommandBuffer;

import helios.ecs.command.tags;
import helios.ecs.common.types;

export namespace helios::ecs::command {

    /**
     * @brief Command buffer implementation that intentionally performs no work.
     */
    class NullCommandBuffer {

    public:

        using EcsRoleTag = tags::CommandBufferRole;
        using InitContextType = ecs::common::types::NullInitContext;
        using FlushContextType = ecs::common::types::NullFlushContext;

        template<class T, class... Args>
        void add(Args&&...) {/*intentionally noop*/}

        bool flush(FlushContextType&) noexcept {return true;}

        bool clear() noexcept {return true;}

        bool init(InitContextType&) noexcept {return true;}

    };


}
