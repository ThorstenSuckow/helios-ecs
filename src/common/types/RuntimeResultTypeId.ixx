/**
 * @file SystemResultTypeId.ixx
 * @brief Unique type identifier for system types.
 */
module;

export module helios.ecs.common.types:RuntimeResultTypeId;

import helios.core.common;

export namespace helios::ecs::common::types {

    struct helios_ecs_common_tag_RuntimeResultTypes {};

    using RuntimeResultTypeId = core::common::types::TypeId<helios_ecs_common_tag_RuntimeResultTypes>;

}; // namespace helios::ecs::common::types