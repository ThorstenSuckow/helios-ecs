/**
 * @file EntityRegistry.ixx
 * @brief Versioned registry for creating, validating, and recycling entity handles.
 */
module;

#include <cassert>
#include <cstddef>
#include <vector>

export module helios.ecs.EntityRegistry;

import helios.ecs.common.types;

import helios.ecs.common.concepts;

using namespace helios::ecs::common::types;
using namespace helios::ecs::common::types;
export namespace helios::ecs {

/**
 * @brief Registry for typed, versioned entity handles.
 *
 * Manages entity creation, liveness checks, version-based stale-handle detection,
 * and index reuse via a free list.
 *
 * @tparam THandle Handle type managed by this registry.
 */
template <typename THandle>
class EntityRegistry {

    /**
     * @brief Recycled entity indices available for reuse.
     */
    std::vector<EntityId> freeList_;

    /**
     * @brief Current version value for each entity slot.
     */
    std::vector<VersionId> versions_;

    /**
     * @brief Reserved capacity for internal storage.
     */
    std::size_t capacity_ = 0;

public:
    /**
     * @brief Constructs an empty registry.
     */
    EntityRegistry() = default;

    /**
     * @brief Constructs a registry with pre-reserved capacity.
     *
     * @param capacity Capacity reserved for internal vectors.
     */
    explicit EntityRegistry(const size_t capacity) {
        reserve(capacity);
    }

    /**
     * @brief Reserves capacity for internal storage.
     *
     * @param capacity New capacity target.
     */
    void reserve(const std::size_t capacity) {
        if (capacity > capacity_) {
            freeList_.reserve(capacity);
            versions_.reserve(capacity);
            capacity_ = capacity;
        }
    }

    /**
     * @brief Creates a new entity handle.
     *
     * Reuses an index from the free list when available; otherwise appends a new slot.
     *
     * @return Valid handle for the created entity.
     */
    THandle create() {

        EntityId idx;
        VersionId version;

        if (freeList_.empty()) {
            idx = static_cast<EntityId>(versions_.size());
            version = InitialVersion;
            versions_.push_back(version);
        } else {
            idx = freeList_.back();
            freeList_.pop_back();

            // version was already incremented in destroy
            version = versions_[idx];
        }

        return {idx, version};
    }

    /**
     * @brief Returns the current version for an entity index.
     *
     * @param entityId Entity index.
     * @return Current version, or `InvalidVersion` if out of bounds.
     */
    [[nodiscard]] VersionId version(const EntityId entityId) const {
        if (entityId >= static_cast<EntityId>(versions_.size())) {
            return InvalidVersion;
        }
        return versions_[entityId];
    }

    /**
     * @brief Returns whether a handle refers to a currently alive entity.
     *
     * @param handle Handle to validate.
     * @return `true` if index is in range and version matches.
     */
    [[nodiscard]] bool isValid(const THandle handle) const noexcept {
        const auto index = handle.entityId();

        if (index >= static_cast<EntityId>(versions_.size())) {
            return false;
        }

        return handle.versionId() == versions_[index];
    }

    /**
     * @brief Destroys an entity and recycles its index.
     *
     * Increments the slot version to invalidate stale handles.
     *
     * @param handle Handle to destroy.
     * @return `true` if destroyed, otherwise `false`.
     */
    bool destroy(const THandle handle) {

        if (!isValid(handle)) {
            return false;
        }

        const auto index = handle.entityId();

        versions_[index] += 1;
        freeList_.push_back(index);

        return true;
    }
};

} // namespace helios::ecs