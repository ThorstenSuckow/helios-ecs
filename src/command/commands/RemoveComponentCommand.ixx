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
template <typename TComponent>
struct RemoveComponentCommand {

    using HandleType = TComponent::HandleType;

    using Component_type = TComponent;

    using CommandGroupType = command::types::CommandGroup<RemoveComponentCommand, HandleType>;

    HandleType handle;

    TComponent component;

    /**
     * @brief Constructs the command with a default-initialised component.
     *
     * @param handle Target entity handle.
     */
    explicit RemoveComponentCommand(HandleType handle)
        requires std::default_initializable<TComponent>
        : handle(handle), component() {}

    /**
     * @brief Constructs the command and forwards arguments to the component constructor.
     *
     * @tparam TArgs Component constructor argument types.
     * @param handle Target entity handle.
     * @param args   Arguments forwarded to `TComponent`.
     */
    template <typename... TArgs>
        requires std::constructible_from<TComponent, TArgs...>
    explicit RemoveComponentCommand(HandleType handle, TArgs&&... args)
        : handle(handle), component(std::forward<TArgs>(args)...) {}
};

}; // namespace helios::ecs::commands