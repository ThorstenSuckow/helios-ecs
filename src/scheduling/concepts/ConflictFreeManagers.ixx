module;

#include <type_traits>

export module helios.ecs.scheduling.concepts:ConflictFreeManagers;

import helios.ecs.scheduling.traits;
import helios.ecs.manager.concepts;
import helios.ecs.common.concepts;
import helios.core.common.types;

export namespace helios::ecs::scheduling::concepts {



    template<typename ... TManagers>
   concept ConflictFreeManagers = (sizeof...(TManagers) >= 2) &&
       (ecs::manager::concepts::IsManagerLike<std::remove_cvref_t<TManagers>> && ...) &&
       !traits::HasConflict<core::common::types::TypeList<std::remove_cvref_t<TManagers>...>>::value;

    template<typename ... TSequential>
    concept ConflictFreeSequentialManagers = (sizeof...(TSequential) >= 1) &&
        (ecs::common::concepts::IsSequentialRuntimeSystemGroup<std::remove_cvref_t<TSequential>> && ...) &&
        !traits::HasConflict<core::common::types::TypeList<std::remove_cvref_t<TSequential>...>>::value;
};
