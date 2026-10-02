module;

#include <type_traits>

export module helios.ecs.scheduling.traits:SequentialAccessSets;

import helios.ecs.common.InvocationContext;
import :SystemAccessSets;
import helios.ecs.system.Sequential;

export namespace helios::ecs::scheduling::traits {


    template<typename TSequential>
    struct SequentialAccessSets;

    template<typename ...TSystems>
    struct SequentialAccessSets<system::Sequential<TSystems...>> {
        using list = core::common::traits::ConcatList<
            typename SystemAccessSets<TSystems>::list ...
        >::list;
    };

};