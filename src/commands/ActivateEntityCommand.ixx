/**
 * @file ActivateEntityCommand.ixx
 * @brief Deferred command for activating an entity.
 */
module;

export module helios.ecs.commands:ActivateEntityCommand;

export namespace helios::ecs::commands {

    /**
     * @brief Deferred command that marks an entity as active.
     *
     * Processed by a command consumer to attach an `Active` component,
     * making the entity visible to views that filter for active entities.
     *
     * @tparam TMemberHandle Entity handle type of the target registry.
     */
    template<typename TMemberHandle>
    struct ActivateEntityCommand {

        /** @brief The handle type identifying the target entity. */
        using Handle_type = TMemberHandle;

        /** @brief Handle of the entity to activate. */
        Handle_type handle;
    };

};