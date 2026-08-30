/**
 * @file CommandBufferRegistry.ixx
 * @brief Type-indexed registry for CommandBuffer instances.
 */
module;

export module helios.ecs.command.CommandBufferRegistry;

import helios.ecs.command.types;

import helios.core.common.container;
import helios.ecs.command.CommandBuffer;

export namespace helios::ecs::command {

/**
 * @brief Type alias for a ConceptModelRegistry specialized for CommandBuffers.
 */
using CommandBufferRegistry =
    helios::core::common::container::ConceptModelRegistry<CommandBuffer, types::CommandBufferTypeId>;

} // namespace helios::ecs::command