/**
 * @file HasExecuteCommandsParallel.ixx
 * @brief Concept detecting an optional `executeParallel` method.
 */
module;

#include <concepts>

export module helios.ecs.manager.concepts:HasExecuteCommandsParallel;

import helios.ecs.common.types;

export namespace helios::ecs::manager::concepts {
    /**
     * @brief Concept detecting an optional `executeParallel` method.
     *
     * @tparam T The manager type to inspect.
     */
    template<typename T>
    concept HasExecuteCommandsParallel = requires {
        &T::executeCommandsParallel;
    };
}