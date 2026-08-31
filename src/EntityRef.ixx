/**
 * @file EntityRef.ixx
 * @brief Type erased Entity reference.
 */
module;

#include <cassert>
#include <exception>

export module helios.ecs.EntityRef;

import helios.ecs.common.types;
import helios.ecs.entity.EntityManager;
import helios.ecs.entity.Entity;

export namespace helios::ecs {

class EntityRef {

    ecs::common::types::HandleTypeId handleId_;
    ecs::common::types::EntityId entityId_;
    ecs::common::types::VersionId versionId_;
    void* entityManager_{};

public:
    template <typename THandle>
    EntityRef(THandle handle, ecs::entity::EntityManager<THandle>& entityManager)
        : handleId_(ecs::common::types::HandleTypeId::template id<THandle>()), entityId_(handle.entityId()),
          versionId_(handle.versionId()), entityManager_(static_cast<void*>(&entityManager)) {}

    template <typename THandle>
    ecs::entity::Entity<ecs::entity::EntityManager<THandle>> get() {

        if (ecs::common::types::HandleTypeId::template id<THandle>() != handleId_) [[unlikely]] {
            assert(false && "EntityRef does not contain the requested handle type.");
            std::terminate();
        }

        return ecs::entity::Entity<ecs::entity::EntityManager<THandle>>{
            THandle{entityId_, versionId_}, static_cast<ecs::entity::EntityManager<THandle>*>(entityManager_)
        };
    }
};

class ConstEntityRef {

    ecs::common::types::HandleTypeId handleId_;
    ecs::common::types::EntityId entityId_;
    ecs::common::types::VersionId versionId_;
    const void* entityManager_{};

public:
    template <typename THandle>
    ConstEntityRef(THandle handle, const ecs::entity::EntityManager<THandle>& entityManager)
        : handleId_(ecs::common::types::HandleTypeId::template id<THandle>()), entityId_(handle.entityId()),
          versionId_(handle.versionId()), entityManager_(static_cast<const void*>(&entityManager)) {}

    template <typename THandle>
    ecs::entity::Entity<const ecs::entity::EntityManager<THandle>> get() {

        if (ecs::common::types::HandleTypeId::template id<THandle>() != handleId_) [[unlikely]] {
            assert(false && "EntityRef does not contain the requested handle type.");
            std::terminate();
        }

        return ecs::entity::Entity<const ecs::entity::EntityManager<THandle>>{
            THandle{entityId_, versionId_}, static_cast<const ecs::entity::EntityManager<THandle>*>(entityManager_)
        };
    }
};

} // namespace helios::ecs