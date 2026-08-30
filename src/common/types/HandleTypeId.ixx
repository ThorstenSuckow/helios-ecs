/**
 * @file HandleTypeId.ixx
 * @brief Runtime-assigned type identifier for context types.
 */
module;

#include <cstddef>

export module helios.ecs.common.types:HandleTypeId;

import helios.core.common;
import helios.core.common.types;

export namespace helios::ecs::common::types {

struct HandleTypeIdDomain {};
using HandleTypeId = helios::core::common::types::TypeId<HandleTypeIdDomain>;

}; // namespace helios::ecs::common::types