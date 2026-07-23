/**
 * @file DeactivateEntityCommand.ixx
 * @brief Deferred command for deactivating an entity.
 */
module;

export module helios.ecs.commands:DeactivateEntityCommand;

export namespace helios::ecs::commands {

    /**
     * @brief Deferred command that marks an entity as inactive.
     *
     * Processed by a command consumer to detach the `Active` component,
     * hiding the entity from views that filter for active entities.
     *
     * @tparam TMemberHandle Entity handle type of the target registry.
     */
    template<typename TMemberHandle>
    struct DeactivateEntityCommand {

        /** @brief The handle type identifying the target entity. */
        using Handle_type = TMemberHandle;

        /** @brief Handle of the entity to deactivate. */
        Handle_type handle;
    };

};