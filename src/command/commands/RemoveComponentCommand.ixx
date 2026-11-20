/**
 * @file RemoveComponentCommand.ixx
 * @brief Deferred command for removing a component from an entity.
 */
module;

#include <concepts>

export module helios.ecs.command.commands:RemoveComponentCommand;

import helios.ecs.command.types;

export namespace helios::ecs::commands {

/**
 * @brief Deferred command that removes a component of type `TComponent` from an entity.
 *
 * @tparam TComponent Component type to detach. Must expose `HandleType`.
 */
template <typename THandle, typename TComponent>
struct RemoveComponentCommand {

    using HandleType = THandle;

    using ComponentType = TComponent;

    HandleType handle;

    /**
     * @brief Constructs the command with the component to remove.
     *
     * @param handle Target entity handle.
     */
    explicit RemoveComponentCommand(HandleType handle)
        : handle(handle) {}

};

}; // namespace helios::ecs::commands