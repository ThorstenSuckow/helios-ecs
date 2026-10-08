module;

#include <type_traits>

export module helios.ecs.scheduling.traits:SystemAccessSets;

import helios.ecs.common.InvocationContext;
import :QueriesToAccessSet;
import helios.ecs.system.traits;

export namespace helios::ecs::scheduling::traits {

    template<typename TSystem>
    struct SystemAccessSets {
        using SystemType = std::remove_cvref_t<TSystem>;

            using InvocationContext = common::SystemInvocationContext<SystemType>::type;

            using list = typename QueriesToAccessSets<
                typename InvocationContext::QueryInfo::list
            >::list;
    };

};