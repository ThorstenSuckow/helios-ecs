module;

#include <type_traits>

export module helios.ecs.scheduling.traits:SystemAccessSets;

import helios.ecs.common.InvocationContext;
import :QueriesToAccessSet;

export namespace helios::ecs::scheduling::traits {

    template<typename TSystem, bool = requires { &TSystem::operator(); }>
    struct SystemInvocationContext;

    template<typename TSystem>
    struct SystemInvocationContext<TSystem, true> {
        using type = common::InvocationContext<
            decltype(&TSystem::operator())
        >;
    };

    template<typename TSystem>
    struct SystemInvocationContext<TSystem, false> {
        using type = common::InvocationContext<
            decltype(&TSystem::update)
        >;
    };

    template<typename TSystem>
    struct SystemAccessSets {
        using SystemType = std::remove_cvref_t<TSystem>;

            using InvocationContext = SystemInvocationContext<SystemType>::type;

            using list = typename QueriesToAccessSets<
                typename InvocationContext::QueryInfo::list
            >::list;
    };

};