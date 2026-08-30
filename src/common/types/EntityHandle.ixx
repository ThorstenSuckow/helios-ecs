/**
 * @file EntityHandle.ixx
 * @brief Versioned handle type for ECS entities.
 */
module;

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>

export module helios.ecs.common.types:EntityHandle;

import :TypeDefs;
import helios.core.common.types;

import :EntityHandleValue;

using namespace helios::ecs::common::types;
using namespace helios::core::common::types;
export namespace helios::ecs::common::types {

/**
 * @brief Sentinel version for invalid or default-initialized handles.
 */
constexpr auto InvalidVersion = VersionId{0};

/**
 * @brief Initial version for newly created entities.
 */
constexpr auto InitialVersion = VersionId{1};

/**
 * @brief Versioned handle used to reference entities in a registry.
 *
 * @tparam TDomainTag Domain tag of the handle type.
 */
template <typename TDomainTag>
class EntityHandle {

    /**
     * @brief Entity identifier in the registry.
     */
    EntityId entityId_{0};

    /**
     * @brief Version used for stale handle detection.
     */
    VersionId versionId_ = InvalidVersion;

public:
    static constexpr EntityHandle from(const EntityHandleValue value) {
        return value.get<EntityHandle>();
    }

    using DomainTag_type = TDomainTag;

    /**
     * @brief Creates an invalid handle.
     */
    EntityHandle() noexcept = default;

    /**
     * @brief Creates a handle from entity and version IDs.
     *
     * @param entityId Entity identifier.
     * @param versionId Version identifier, must not be `InvalidVersion`.
     */
    EntityHandle(const EntityId entityId, const VersionId versionId) noexcept
        : entityId_(entityId), versionId_(versionId) {
        assert(versionId_ != InvalidVersion && "EntityHandle must not be constructed with InvalidVersion.");
    }

    /**
     * @brief Returns the entity ID.
     *
     * @return Entity ID.
     */
    [[nodiscard]] EntityId entityId() const noexcept {
        return entityId_;
    }

    /**
     * @brief Returns the version ID.
     *
     * @return Version ID.
     */
    [[nodiscard]] VersionId versionId() const noexcept {
        return versionId_;
    }

    /**
     * @brief Compares two handles for equality.
     */
    bool operator==(const EntityHandle<TDomainTag>&) const = default;

    /**
     * @brief Compares two handles by entity ID (less-than).
     *
     * @param entityHandle Handle to compare with.
     * @return `true` if this handle has a smaller entity ID.
     */
    constexpr bool operator<(const EntityHandle<TDomainTag>& entityHandle) const noexcept {
        return entityId_ < entityHandle.entityId_;
    }

    /**
     * @brief Compares two handles by entity ID (greater-than).
     *
     * @param entityHandle Handle to compare with.
     * @return `true` if this handle has a greater entity ID.
     */
    constexpr bool operator>(const EntityHandle<TDomainTag>& entityHandle) const noexcept {
        return entityId_ > entityHandle.entityId_;
    }

    /**
     * @brief Returns whether this handle is valid.
     *
     * @return `true` if the version is not `InvalidVersion`.
     */
    [[nodiscard]] bool isValid() const noexcept {
        return versionId_ != InvalidVersion;
    }

    [[nodiscard]] EntityHandleValue value() const noexcept {
        return EntityHandleValue(*this);
    }
};

} // namespace helios::ecs::common::types

/**
 * @brief Hash specialization for `EntityHandle`.
 *
 * Hashes a packed `(entityId, versionId)` 64-bit value.
 */
template <typename TDomainTag>
struct std::hash<EntityHandle<TDomainTag>> {
    std::size_t operator()(const EntityHandle<TDomainTag>& handle) const noexcept {

        const uint64_t packed =
            (static_cast<uint64_t>(handle.entityId) << 32) | static_cast<uint64_t>(handle.versionId);

        return std::hash<uint64_t>{}(packed);
    }
};
