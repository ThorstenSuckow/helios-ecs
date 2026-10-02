/**
 * @file IsSequentialSystemGroupContext.ixx
 * @brief Concepts and helpers for grouping and constraining ECS systems by handle type.
 */
module;

#include <concepts>

export module helios.ecs.system.concepts:IsSequentialSystemGroupContext;

import helios.ecs.system.Sequential;

export namespace helios::ecs::system::concepts {


    template <typename T>
    struct SequentialSystemGroup : std::false_type {};

    template <typename TFirstSystem, typename... TSystems>
    struct SequentialSystemGroup<Sequential<TFirstSystem, TSystems...>> : std::true_type {};

    template <typename ... TSystems>
    concept IsSequentialSystemGroup = SequentialSystemGroup<std::remove_cvref_t<TSystems>...>::value;

}; // namespace helios::ecs::system::concepts