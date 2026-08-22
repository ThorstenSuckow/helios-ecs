module;

#include <concepts>

/**
 * @file System.ixx
 * @brief Concepts and helpers for grouping and constraining ECS systems by handle type.
 */
export module helios.ecs.system.concepts:SystemConcepts;

import helios.ecs.common.concepts;

export namespace helios::ecs::system::concepts {


    /**
     * @brief Concept that constrains a type to a TypedSystem-like type.
     *
     * @tparam T The type to inspect.
     */
    template<class T>
    concept IsEcsSystemLike = requires{&std::remove_cvref_t<T>::update;};

    template<class T>
    concept IsCallableSystem = requires {&std::remove_cvref_t<T>::operator();};


};