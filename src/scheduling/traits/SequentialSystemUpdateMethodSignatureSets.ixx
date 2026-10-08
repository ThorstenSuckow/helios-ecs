module;

#include <type_traits>

export module helios.ecs.scheduling.traits:SequentialSystemUpdateMethodSignatureSets;

import helios.ecs.common.InvocationContext;
import helios.core.common.traits;
import helios.core.common.types;

import :SystemToUpdateMethodSignature;
import helios.ecs.system.Sequential;

export namespace helios::ecs::scheduling::traits {

    template<typename TSequential>
    struct SequentialSystemUpdateMethodSignatureSets;

    template<typename ...TSystems>
    struct SequentialSystemUpdateMethodSignatureSets<system::Sequential<TSystems...>> {
        using list = core::common::types::TypeList<
            typename SystemToUpdateMethodSignature<TSystems>::MethodSignature ...
        >;
    };

};