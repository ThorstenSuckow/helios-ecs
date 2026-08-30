/**
 * @file SystemTypeId.ixx
 * @brief Unique type identifier for system types.
 */
module;

export module helios.ecs.system.types:SystemTypeId;

import helios.core.common;

export namespace helios::ecs::system::types {

struct helios_ecs_system_tag_SystemTypes {};

using SystemTypeId = core::common::types::TypeId<helios_ecs_system_tag_SystemTypes>;

}; // namespace helios::ecs::system::types