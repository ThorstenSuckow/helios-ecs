/**
 * @file EntitySpanRef.ixx
 * @brief Type erased span of Entities.
 */
module;


#include <span>
#include <cassert>
#include <exception>

export module helios.ecs.EntitySpanRef;


import helios.ecs.Entity;
import helios.ecs.EntityManager;
import helios.ecs.common.types;



export namespace helios::ecs {

    class EntitySpanRef {

        common::types::HandleTypeId typeId_;

        void* data_{};
        std::size_t size_{};

        template<typename THandle>
        using EntityType = Entity<EntityManager<THandle>>;

    public:

        template <typename THandle>
        explicit EntitySpanRef(std::vector<EntityType<THandle>>& entities)
        : EntitySpanRef{std::span<EntityType<THandle>>(entities)} {}

        template <typename THandle>
        explicit EntitySpanRef(std::span<EntityType<THandle>> entities)
        : typeId_{ecs::common::types::HandleTypeId::template id<THandle>()},
          data_{entities.data()},
          size_{entities.size()}
        {}

        template<typename THandle>
        [[nodiscard]] std::span<EntityType<THandle>> get() const {

            if (typeId_ != ecs::common::types::HandleTypeId::template id<THandle>()) [[unlikely]] {
                assert(false && "Unexpected TypeId mismatch");
                std::terminate();
            }
            return {
                static_cast<EntityType<THandle>*>(data_),
                size_
            };
        }


    };


}