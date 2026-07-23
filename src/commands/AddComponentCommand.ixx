/**
 * @file AddComponentCommand.ixx
 * @brief Deferred command for adding a component to an entity.
 */
module;

#include <memory>

export module helios.ecs.commands:AddComponentCommand;

export namespace helios::ecs::commands {


    /**
     * @brief Deferred command that adds a component of type `TComponent` to an entity.
     *
     * Intended to be queued and processed by a command consumer (e.g. a system
     * or entity manager) rather than applied immediately, keeping structural
     * ECS mutations out of hot iteration loops.
     *
     * @tparam TComponent Component type to attach. Must expose `Handle_type`.
     */
    template<typename TComponent>
    struct AddComponentCommand {

        /** @brief Handle type derived from the component. */
        using Handle_type = TComponent::Handle_type;

        /** @brief The component type being added. */
        using Component_type = TComponent;

        /** @brief Handle of the target entity. */
        Handle_type handle;

        /** @brief Component instance that will be attached. */
        TComponent component;

        /**
         * @brief Constructs the command with a default-initialised component.
         *
         * @param handle Target entity handle.
         */
        explicit AddComponentCommand(Handle_type handle)
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
        explicit AddComponentCommand(Handle_type handle, TArgs&& ...args)
        : handle(handle), component(std::forward<TArgs>(args)...) {}

    };

};