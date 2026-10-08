module;

#include <type_traits>

export module helios.ecs.scheduling.traits:SystemToUpdateMethodSignature;

import helios.ecs.common.InvocationContext;
import helios.ecs.system.traits;
import helios.ecs.system.types;


export namespace helios::ecs::scheduling::traits {



    template<typename TSystem>
    struct SystemToUpdateMethodSignature {
        using SystemType = std::remove_cvref_t<TSystem>;

        using InvocationContext = common::SystemInvocationContext<SystemType>::type;

        using MethodSignature = typename InvocationContext::UpdateMethodSignature;
        using ReturnType = typename MethodSignature::ReturnType;
        using ArgumentTypeList = typename MethodSignature::ArgumentTypeList;
    };

};