/**
 * @file System.ixx
 * @brief Type-erased system wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>
#include <optional>
#include <variant>

export module helios.ecs.common.RuntimeResult;

import helios.core.common.types;
import helios.ecs.common.types;

export namespace helios::ecs::common {

    using RuntimeResult = helios::core::common::types::TypeErasedValueWrapper<types::RuntimeResultTypeId>;

}