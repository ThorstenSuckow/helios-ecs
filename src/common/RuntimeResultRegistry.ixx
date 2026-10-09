/**
 * @file RuntimeResultRegistry.ixx
 * @brief Type-indexed registry for managing RuntimeResult instances within a game loop pass.
 */
module;

#include <cassert>

export module helios.ecs.common.RuntimeResultRegistry;

import helios.ecs.common.RuntimeResult;
import helios.ecs.common.types;

import helios.core.common.container;

export namespace helios::ecs::common {

/**
 * @brief Type alias for a ConceptModelRegistry specialized for RuntimeResults.
 *
 * @see ConceptModelRegistry
 * @see RuntimeResult
 */
using RuntimeResultRegistry = core::common::container::ConceptModelRegistry<
    RuntimeResult, RuntimeResult::TypeId>;

} // namespace helios::ecs::common