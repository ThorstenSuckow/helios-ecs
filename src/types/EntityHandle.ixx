/**
 * @file EntityHandle.ixx
 * @brief Versioned handle type for ECS entities.
 */
module;


#include <functional>
#include <cstdint>
#include <cstddef>
#include <cassert>

export module helios.ecs.types.EntityHandle;

import helios.ecs.types.TypeDefs;
import helios.ecs.types.StrongId;

using namespace helios::ecs::types;
export namespace helios::ecs::types {

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
     * A handle is valid when its version is not `InvalidVersion`.
     * Stale handles are detected via the version value.
     *
     * @tparam TDomainTag Domain tag of the handle type.
     */
    template<typename TDomainTag>
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
        EntityHandle(const EntityId entityId, const VersionId versionId) noexcept : entityId_(entityId), versionId_(versionId) {
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

    };

}


/**
 * @brief Hash specialization for `EntityHandle`.
 *
 * Hashes a packed `(entityId, versionId)` 64-bit value.
 */
template<typename TDomainTag>
struct std::hash<helios::ecs::types::EntityHandle<TDomainTag>> {
    std::size_t operator()(const helios::ecs::types::EntityHandle<TDomainTag>& handle) const noexcept {

        const uint64_t packed = (static_cast<uint64_t>(handle.entityId) << 32) |
                                static_cast<uint64_t>(handle.versionId);

        return std::hash<uint64_t>{}(packed);
    }
};
