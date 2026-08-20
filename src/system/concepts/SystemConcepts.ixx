module;

#include <concepts>

/**
 * @file System.ixx
 * @brief Concepts and helpers for grouping and constraining ECS systems by handle type.
 */
export module helios.ecs.system.concepts:SystemConcepts;

import helios.ecs.system.tags;
import helios.ecs.common.concepts;

export namespace helios::ecs::system::concepts {




    /**
     * @brief Concept that constrains a type to a TypedSystem-like type.
     *
     * @tparam T The type to inspect.
     */
    template<class T>
    concept IsTypedSystemLike = common::concepts::HasEcsTag<T, tags::TypedSystemRole>;


    /**
     * @brief Concept that constrains a type to a callable-backed system.
     *
     * @tparam T The type to inspect.
     */
    template<class T>
     concept IsCallableSystemLike = common::concepts::HasEcsTag<T, tags::CallableSystemRole>;

    template<class T>
    concept IsRuntimeSystemLike = IsTypedSystemLike<T> || IsCallableSystemLike<T>;

};