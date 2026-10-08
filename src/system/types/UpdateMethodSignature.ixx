/**
* @file UpdateMethodSignature.ixx
 * @brief Trait for determining the types and positions of update() arguments, excluding Queries..
 */
module;

#include <tuple>


export module helios.ecs.system.types:UpdateMethodSignature;

import helios.ecs.entity.concepts;
import helios.core.common.traits;
import helios.core.common.types;
import helios.ecs.entity.query.NullQuery;

export namespace helios::ecs::system::types {

    template <typename... TArgs>
    struct UpdateMethodSignature;

    template <typename TReturnType, typename ... TArguments>
    struct UpdateMethodSignature<TReturnType, core::common::types::TypeList<TArguments...>> {
        using ReturnType = TReturnType;
        using ArgumentTypeList = core::common::types::TypeList<TArguments...>;
    };

    template <typename TReturnType, typename ... TArguments>
    struct UpdateMethodSignature<TReturnType, std::tuple<TArguments...>> :
            UpdateMethodSignature<TReturnType, core::common::types::TypeList<TArguments...>>{};

}