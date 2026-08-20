/**
 * @file IsCommandBufferLike.ixx
 * @brief Concept constraining types usable as command buffers.
 */
module;

#include <concepts>

export module helios.ecs.command.concepts:IsCommandBufferLike;


import helios.ecs.common.concepts;
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
        TBuffer& buffer
    )
    {
        requires ecs::common::concepts::HasEcsTag<TBuffer, tags::CommandBufferRole>;
    };
}
