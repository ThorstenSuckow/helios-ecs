module;

#include <type_traits>

export module helios.ecs.scheduling.traits:RuntimeSystemToMainMethodSignature;

import helios.ecs.common.InvocationContext;
import helios.ecs.system.types;


export namespace helios::ecs::scheduling::traits {



    template<typename TSystem>
    struct RuntimeSystemToMainMethodSignature {
        using SystemType = std::remove_cvref_t<TSystem>;

        using InvocationContext = common::RuntimeSystemInvocationContext<SystemType>::type;

        using MethodSignature = typename InvocationContext::RuntimeMainMethodSignature;
        using ReturnType = typename MethodSignature::ReturnType;
        using ArgumentTypeList = typename MethodSignature::ArgumentTypeList;
    };

};