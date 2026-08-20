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
     * @brief Concept detecting an optional `executeCommandsParallel` method.
     *
     * @tparam TManager The manager type to inspect.
     * @tparam TExecutionContext The execution context type to pass to `executeCommandsParallel`.
     */
    template<typename TManager, typename TExecutionContext>
    concept HasExecuteCommandsParallel = requires (TManager& m, TExecutionContext& executionContext) {
        {m.executeCommandsParallel(executionContext)} -> std::same_as<bool>;
    };
}