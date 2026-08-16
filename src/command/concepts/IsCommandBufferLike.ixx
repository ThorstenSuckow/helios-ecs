/**
 * @file IsCommandBufferLike.ixx
 * @brief Concept constraining types usable as command buffers.
 */
module;

#include <concepts>

export module helios.ecs.command.concepts:IsCommandBufferLike;

import helios.ecs.command.tags;

export namespace helios::ecs::command::concepts {

    /**
     * @brief Constrains types that can serve as concrete command buffers.
     *
     * @tparam T The type to constrain.
     *
     * @see CommandBuffer
     * @see CommandBufferRole
     * @see HasClear
     */
    template<class TBuffer>
    concept IsCommandBufferLike = requires (
        TBuffer& buffer,
        typename TBuffer::InitContextType& initCtx,
        typename TBuffer::FlushContextType& flushCtx
    )
    {
        std::same_as<typename TBuffer::EcsRoleTag, tags::CommandBufferRole>;
        {buffer.init(initCtx)} -> std::same_as<bool>;
        {buffer.flush(flushCtx)} -> std::same_as<bool>;
        {buffer.clear()} -> std::same_as<bool>;

    };
}
