/**
 * @file HasExecuteParallel.ixx
 * @brief Concept detecting an optional `executeParallel` method.
 */
module;

#include <concepts>

export module helios.ecs.manager.concepts:HasExecuteParallel;

import helios.ecs.types;

export namespace helios::ecs::manager::concepts {
    /**
     * @brief Concept detecting an optional `executeParallel` method.
     *
     * @tparam T The manager type to inspect.
     */
    template<typename T>
    concept HasExecuteParallel = requires {
        &T::executeParallel;
    };
}