module;

#include <type_traits>

export module helios.ecs.scheduling.traits:RuntimeSystemAccessSets;

import helios.ecs.common.InvocationContext;
import :QueriesToAccessSet;

export namespace helios::ecs::scheduling::traits {

    template<typename TSystem>
    struct RuntimeSystemAccessSets {
        using SystemType = std::remove_cvref_t<TSystem>;

            using InvocationContext = common::RuntimeSystemInvocationContext<SystemType>::type;

            using list = typename QueriesToAccessSets<
                typename InvocationContext::QueryInfo::list
            >::list;
    };

};