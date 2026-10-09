/**
 * @file IsSequentialSystemGroupContext.ixx
 * @brief Concepts and helpers for grouping and constraining ECS systems by handle type.
 */
module;

#include <concepts>

export module helios.ecs.common.concepts:IsSequentialRuntimeSystemGroupContext;

import helios.ecs.common.SequentialRuntimeSystemGroup;

export namespace helios::ecs::common::concepts {


    template <typename T>
    struct SequentialRuntimeSystemGroup : std::false_type {};

    template <typename TFirstSystem, typename... TSystems>
    struct SequentialRuntimeSystemGroup<Sequential<TFirstSystem, TSystems...>> : std::true_type {};

    template <typename ... TSystems>
    concept IsSequentialRuntimeSystemGroup = SequentialRuntimeSystemGroup<std::remove_cvref_t<TSystems>...>::value;

}; // namespace helios::ecs::common::concepts