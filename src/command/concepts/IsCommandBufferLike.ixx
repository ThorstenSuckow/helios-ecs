/**
 * @file IsCommandBufferLike.ixx
 * @brief Concept constraining types usable as command buffers.
 */
module;

#include <concepts>

export module helios.ecs.command.concepts:IsCommandBufferLike;


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
    template<class T>
    concept IsCommandBufferLike = true;
}
