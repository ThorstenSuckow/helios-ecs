module;

#include <type_traits>

export module helios.ecs.scheduling.traits:SequentialAccessSets;

import helios.ecs.common.InvocationContext;
import :RuntimeSystemAccessSets;
import helios.ecs.common.SequentialRuntimeSystemGroup;

export namespace helios::ecs::scheduling::traits {


    template<typename TSequential>
    struct SequentialAccessSets;

    template<typename ...TSystems>
    struct SequentialAccessSets<common::Sequential<TSystems...>> {
        using list = core::common::traits::ConcatList<
            typename RuntimeSystemAccessSets<TSystems>::list ...
        >::list;
    };

};