/**
* @file UpdateMethodSignature.ixx
 * @brief Trait for determining the types and positions of update() arguments, excluding Queries..
 */
module;

#include <tuple>


export module helios.ecs.scheduling.types:RuntimeMainMethodSignature;

import helios.ecs.entity.concepts;
import helios.core.common.traits;
import helios.core.common.types;
import helios.ecs.entity.query.NullQuery;

export namespace helios::ecs::scheduling::types {

    template <typename... TArgs>
    struct RuntimeMainMethodSignature;

    template <typename TReturnType, typename ... TArguments>
    struct RuntimeMainMethodSignature<TReturnType, core::common::types::TypeList<TArguments...>> {
        using ReturnType = TReturnType;
        using ArgumentTypeList = core::common::types::TypeList<TArguments...>;
    };

    template <typename TReturnType, typename ... TArguments>
    struct RuntimeMainMethodSignature<TReturnType, std::tuple<TArguments...>> :
            RuntimeMainMethodSignature<TReturnType, core::common::types::TypeList<TArguments...>>{};

}