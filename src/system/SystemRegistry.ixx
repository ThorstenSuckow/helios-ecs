/**
 * @file SystemRegistry.ixx
 * @brief Type-indexed registry for managing System instances within a game loop pass.
 */
module;

#include <cassert>
#include <memory>
#include <span>
#include <vector>


export module helios.ecs.system.SystemRegistry;

import helios.ecs.system.System;
import helios.ecs.system.types;

import helios.core.common.container;

export namespace helios::ecs::system {

    /**
     * @brief Type alias for a ConceptModelRegistry specialized for Systems.
     *
     * @see ConceptModelRegistry
     * @see System
     */
    using SystemRegistry = core::common::container::ConceptModelRegistry<System, types::SystemTypeId>;

}