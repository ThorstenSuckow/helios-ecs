/**
 * @file IsManagerLike.ixx
 * @brief Concept constraining types eligible for manager registration.
 */
module;


export module helios.ecs.manager.concepts:IsManagerLike;

import helios.ecs.manager.tags;
import helios.ecs.command.concepts;
import helios.ecs.common.concepts;

export namespace helios::ecs::manager::concepts {

    /**
     * @brief Constrains T to types that provide `flush(UpdateContext&)` and
     *        declare `EcsRoleTag = ManagerRole`.
     *
     * @tparam TManager The manager type to constrain.
     */
    template<class TManager>
    concept IsManagerLike = requires (
        TManager& manager
    )
    {
        requires ecs::command::concepts::IsCommandTypeList<typename TManager::CommandTypes>::value;
        requires ecs::common::concepts::HasEcsTag<TManager, tags::ManagerRole>;
    };
}
