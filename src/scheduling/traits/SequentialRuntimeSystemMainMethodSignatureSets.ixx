module;

#include <type_traits>

export module helios.ecs.scheduling.traits:SequentialRuntimeSystemMainMethodSignatureSets;

import helios.ecs.common.InvocationContext;
import helios.core.common.traits;
import helios.core.common.types;

import :RuntimeSystemToMainMethodSignature;
import helios.ecs.common.SequentialRuntimeSystemGroup;

export namespace helios::ecs::scheduling::traits {

    template<typename TSequential>
    struct SequentialRuntimeSystemMainMethodSignatureSets;

    template<typename ...TSystems>
    struct SequentialRuntimeSystemMainMethodSignatureSets<common::Sequential<TSystems...>> {
        using list = core::common::types::TypeList<
            typename RuntimeSystemToMainMethodSignature<TSystems>::MethodSignature ...
        >;
    };

};