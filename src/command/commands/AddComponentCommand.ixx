/**
 * @file AddComponentCommand.ixx
 * @brief Deferred command for adding a component to an entity.
 */
module;

#include <concepts>

export module helios.ecs.command.commands:AddComponentCommand;

import helios.ecs.command.types;

export namespace helios::ecs::commands {

/**
 * @brief Deferred command that adds a component of type `TComponent` to an entity.
 *
 * @tparam TComponent Component type to attach. Must expose `HandleType`.
 */
template <typename TComponent>
struct AddComponentCommand {

    using HandleType = TComponent::HandleType;

    using ComponentType = TComponent;

    HandleType handle;

    TComponent component;

    /**
     * @brief Constructs the command with a default-initialised component.
     *
     * @param handle Target entity handle.
     */
    explicit AddComponentCommand(HandleType handle)
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
    explicit AddComponentCommand(HandleType handle, TArgs&&... args)
        : handle(handle), component(std::forward<TArgs>(args)...) {}


};

}; // namespace helios::ecs::commands