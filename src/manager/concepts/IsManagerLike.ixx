/**
 * @file IsManagerLike.ixx
 * @brief Concept constraining types eligible for manager registration.
 */
module;

#include <concepts>

export module helios.ecs.manager.concepts:IsManagerLike;


export namespace helios::ecs::manager::concepts {

    /**
     * @brief Constrains T to types that provide `flush(ecs::types::UpdateContext&)` and
     *        declare `EcsRoleTag = ManagerRole`.
     *
     * @tparam TManager The manager type to constrain.
     */
    template<class TManager>
    concept IsManagerLike = true;
}
