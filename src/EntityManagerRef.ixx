/**
 * @file EntityManagerRef.ixx
 * @brief Type erased EntityManager reference.
 */
module;

#include <cassert>
#include <exception>

export module helios.ecs.EntityManagerRef;

import helios.ecs.common.types;
import helios.ecs.entity.EntityManager;
import helios.ecs.entity.Entity;

export namespace helios::ecs {

class EntityManagerRef {

    ecs::common::types::HandleTypeId handleId_;
    void* entityManager_{};

public:
    template <typename THandle>
    EntityManagerRef(ecs::entity::EntityManager<THandle>& entityManager)
        : handleId_(ecs::common::types::HandleTypeId::template id<THandle>()),
          entityManager_(static_cast<void*>(&entityManager)) {}

    template <typename THandle>
    ecs::entity::EntityManager<THandle>& get() const {

        if (ecs::common::types::HandleTypeId::template id<THandle>() != handleId_) [[unlikely]] {
            assert(false && "EntityRef does not contain the requested handle type.");
            std::terminate();
        }

        return *static_cast<ecs::entity::EntityManager<THandle>*>(entityManager_);
    }
};

} // namespace helios::ecs