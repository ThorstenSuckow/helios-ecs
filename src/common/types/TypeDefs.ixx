/**
 * @file TypeDefs.ixx
 * @brief Core type definitions and tag types for the helios engine.
 */
module;

#include <cstdint>
#include <limits>
#include <cstddef>

export module helios.ecs.common.types:TypeDefs;


export namespace helios::ecs::common::types {


    /**
     * @brief Version number for detecting stale entity references.
     */
    using VersionId = uint32_t;

    /**
     * @brief Unique identifier for an entity within an EntityPool.
     */
    using EntityId = uint32_t;


    /**
     * @brief Sentinel value indicating an invalid or removed sparse index.
     */
    constexpr EntityId EntityTombstone = std::numeric_limits<EntityId>::max();



}