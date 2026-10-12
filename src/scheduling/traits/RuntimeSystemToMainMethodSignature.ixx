module;

#include <type_traits>

export module helios.ecs.scheduling.traits:RuntimeSystemToMainMethodSignature;

import :RuntimeSystemMainMethodArgumentSelector;
import helios.ecs.common.InvocationContext;
import helios.ecs.system.types;
import helios.ecs.scheduling.types;


export namespace helios::ecs::scheduling::traits {



    template<typename TSystem>
    struct RuntimeSystemToMainMethodSignature {
        using SystemType = std::remove_cvref_t<TSystem>;

        using InvocationContext = common::RuntimeSystemInvocationContext<SystemType>::type;

        using MethodSignature = types::RuntimeMainMethodSignature<
            typename InvocationContext::RuntimeMainMethodSignature::ReturnType,
            typename RuntimeSystemMainMethodArgumentSelectorFromList<
                typename InvocationContext::RuntimeMainMethodSignature::ArgumentTypeList
            >::list
        >;
        using ReturnType = typename MethodSignature::ReturnType;
        using ArgumentTypeList = typename MethodSignature::ArgumentTypeList;
    };

};