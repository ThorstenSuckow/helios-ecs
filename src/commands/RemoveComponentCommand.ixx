/**
 * @file RemoveComponentCommand.ixx
 * @brief Deferred command for removing a component from an entity.
 */
module;

#include <memory>

export module helios.ecs.commands:RemoveComponentCommand;

export namespace helios::ecs::commands {


    /**
     * @brief Deferred command that removes a component of type `TComponent` from an entity.
     *
     * Mirrors `AddComponentCommand` in structure. The stored component
     * instance may carry contextual data needed by the consumer during removal.
     *
     * @tparam TComponent Component type to detach. Must expose `Handle_type`.
     */
    template<typename TComponent>
    struct RemoveComponentCommand {

        /** @brief Handle type derived from the component. */
        using Handle_type = TComponent::Handle_type;

        /** @brief The component type being removed. */
        using Component_type = TComponent;

        /** @brief Handle of the target entity. */
        Handle_type handle;

        /** @brief Component instance providing context for the removal. */
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