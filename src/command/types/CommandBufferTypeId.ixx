/**
 * @file CommandBufferTypeId.ixx
 * @brief Unique type identifier for command buffers.
 */
module;

#include <cstddef>

export module helios.ecs.command.types:CommandBufferTypeId;

import helios.core;

export namespace helios::ecs::command::types {

struct CommandBufferTypesIdDomain {};

using CommandBufferTypeId = helios::core::common::types::TypeId<CommandBufferTypesIdDomain>;

}; // namespace helios::ecs::command::types