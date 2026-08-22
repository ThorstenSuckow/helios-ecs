/**
 * @file ContextTypeId.ixx
 * @brief Runtime-assigned type identifier for context types.
 */
module;

#include <functional>
#include <cstddef>

export module helios.ecs.common.types:ContextTypeId;

import helios.core.common;

export namespace helios::ecs::common::types {

    struct RuntimeContextTypeIdDomain{};

    using ContextTypeId = helios::core::common::types::TypeId<RuntimeContextTypeIdDomain>;

}