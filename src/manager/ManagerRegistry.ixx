/**
 * @file ManagerRegistry.ixx
 * @brief Type-indexed registry for managing Manager instances.
 */
module;

export module helios.ecs.manager.ManagerRegistry;

import helios.core.container;
import helios.ecs.manager.Manager;
import helios.ecs.manager.types;

export namespace helios::ecs::manager {

    /**
     * @brief Type alias for a ConceptModelRegistry specialized for Managers.
     *
     * @see ConceptModelRegistry
     * @see Manager
     */
    using ManagerRegistry = core::container::ConceptModelRegistry<Manager, types::ManagerTypeId>;

}