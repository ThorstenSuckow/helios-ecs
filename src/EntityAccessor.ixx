
module;

#include <optional>

export module helios.ecs.EntityAccessor;

import helios.ecs.EntityManager;
import helios.ecs.Entity;

import helios.ecs.common.types;

using namespace helios::ecs::common::types;
export namespace helios::ecs {

    template<typename THandle>
    class EntityAccessor {

        
        /**
         * @brief Non-owning pointer to the EntityManager used for validation.
         */
        EntityManager<THandle>* const em;

    public:

        explicit EntityAccessor(EntityManager<THandle>* em) noexcept
            : em(em) {}

    };
}