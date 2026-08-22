/**
 * @file IsCommandBufferLike.ixx
 * @brief Concept constraining types usable as command buffers.
 */
module;

#include <concepts>

export module helios.ecs.command.concepts:IsCommandBufferLike;


import helios.ecs.common.concepts;


export namespace helios::ecs::command::concepts {

    template<class TBuffer>
    concept IsCommandBufferLike = requires
    {
        &std::remove_cvref_t<TBuffer>::flush;
        &std::remove_cvref_t<TBuffer>::clear;
    };
}
