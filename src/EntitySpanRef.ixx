/**
 * @file EntitySpanRef.ixx
 * @brief Type erased span of Entities.
 */
module;

#include <cassert>
#include <exception>
#include <span>

export module helios.ecs.EntitySpanRef;

import helios.ecs.entity.Entity;
import helios.ecs.entity.EntityManager;
import helios.ecs.common.types;

export namespace helios::ecs {

class EntitySpanRef {

    common::types::HandleTypeId typeId_;

    void* data_{};
    std::size_t size_{};

    template <typename THandle>
    using EntityType = entity::Entity<entity::EntityManager<THandle>>;

public:
    template <typename THandle>
    explicit EntitySpanRef(std::vector<EntityType<THandle>>& entities)
        : EntitySpanRef{std::span<EntityType<THandle>>(entities)} {}

    template <typename THandle>
    explicit EntitySpanRef(std::span<EntityType<THandle>> entities)
        : typeId_{ecs::common::types::HandleTypeId::template id<THandle>()}, data_{entities.data()},
          size_{entities.size()} {}

    template <typename THandle>
    [[nodiscard]] std::span<EntityType<THandle>> get() const {

        if (typeId_ != ecs::common::types::HandleTypeId::template id<THandle>()) [[unlikely]] {
            assert(false && "Unexpected TypeId mismatch");
            std::terminate();
        }
        return {static_cast<EntityType<THandle>*>(data_), size_};
    }
};

} // namespace helios::ecs