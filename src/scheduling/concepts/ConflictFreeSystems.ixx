module;

#include <type_traits>

export module helios.ecs.scheduling.concepts:ConflictFreeSystems;

import helios.ecs.scheduling.traits;
import helios.ecs.system.concepts;
import helios.core.common.types;

export namespace helios::ecs::scheduling::concepts {

    template<typename ... TSystems>
    concept ConflictFreeCallableSystems = (sizeof...(TSystems) >= 2) &&
        (ecs::system::concepts::IsCallableSystem<std::remove_cvref_t<TSystems>> && ...) &&
        !traits::HasConflict<core::common::types::TypeList<std::remove_cvref_t<TSystems>...>>::value;

    template<typename ... TSystems>
    concept ConflictFreeTypedSystems = (sizeof...(TSystems) >= 2) &&
        (ecs::system::concepts::IsTypedSystem<std::remove_cvref_t<TSystems>> && ...) &&
        !traits::HasConflict<core::common::types::TypeList<std::remove_cvref_t<TSystems>...>>::value;

    template<typename ... TSequential>
    concept ConflictFreeSequentialSystems = (sizeof...(TSequential) >= 1) &&
        (ecs::system::concepts::IsSequentialSystemGroup<std::remove_cvref_t<TSequential>> && ...) &&
        !traits::HasConflict<core::common::types::TypeList<std::remove_cvref_t<TSequential>...>>::value;
};
