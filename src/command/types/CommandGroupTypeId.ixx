/**
 * @file CommandGroupTypeId.ixx
 * @brief Unique type identifier for command group types.
 */
module;

#include <cstddef>

export module helios.ecs.command.types:CommandGroupTypeId;

import helios.core.common;

export namespace helios::ecs::command::types {

struct helios_ecs_tag_CommandGroupTypes {};
using CommandGroupTypeId = helios::core::common::types::TypeId<helios_ecs_tag_CommandGroupTypes>;

}; // namespace helios::ecs::command::types