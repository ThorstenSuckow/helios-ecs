/**
 * @file ComponentTypeId.ixx
 * @brief Compile-time type identifier for component types.
 *
 * @details Provides a unique, compile-time generated ID for each component type,
 * enabling O(1) direct indexing into the component storage vector within GameObject.
 */
module;

#include <functional>
#include <cstddef>

export module helios.ecs.common.types:ComponentTypeId;

import helios.core.common;

export namespace helios::ecs::common::types {
    template<typename THandle>
    using ComponentTypeId = helios::core::common::types::TypeId<THandle>;
};