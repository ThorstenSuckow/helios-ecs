/**
 * @file RemoveComponentCommand.ixx
 * @brief Deferred command for removing a component from an entity.
 */
module;

#include <memory>

export module helios.ecs.common.commands:RemoveComponentCommand;

import helios.ecs.command.types;

export namespace helios::ecs::common::commands {


    /**
     * @brief Deferred command that removes a component of type `TComponent` from an entity.
     *
     * @tparam TComponent Component type to detach. Must expose `Handle_type`.
     */
    template<typename TComponent>
    struct RemoveComponentCommand {

        using Handle_type = TComponent::Handle_type;

        using Component_type = TComponent;

        using Group_type = command::types::CommandGroup<RemoveComponentCommand, Handle_type>;

        Handle_type handle;

        TComponent component;

        /**
         * @brief Constructs the command with a default-initialised component.
         *
         * @param handle Target entity handle.
         */
        explicit RemoveComponentCommand(Handle_type handle)
        requires std::default_initializable<TComponent>
        : handle(handle), component() {}

        /**
         * @brief Constructs the command and forwards arguments to the component constructor.
         *
         * @tparam TArgs Component constructor argument types.
         * @param handle Target entity handle.
         * @param args   Arguments forwarded to `TComponent`.
         */
        template<typename ...TArgs>
        requires std::constructible_from<TComponent, TArgs...>
        explicit RemoveComponentCommand(Handle_type handle, TArgs&& ...args)
        : handle(handle), component(std::forward<TArgs>(args)...) {}

    };

};