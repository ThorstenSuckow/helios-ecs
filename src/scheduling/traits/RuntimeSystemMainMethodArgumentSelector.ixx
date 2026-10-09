/**
 * @file UpdateMethodArguments.ixx
 * @brief Trait for determining the types and positions of update() arguments, excluding Queries..
 */
module;

#include <type_traits>
#include <tuple>


export module helios.ecs.scheduling.traits:RuntimeSystemMainMethodArgumentSelector;

import helios.ecs.entity.concepts;
import helios.core.common.traits;
import helios.core.common.types;
import helios.ecs.entity.query.NullQuery;

export namespace helios::ecs::scheduling::traits {

    template <typename... TArgs>
    struct RuntimeSystemMainMethodArgumentSelector;

    template <>
    struct RuntimeSystemMainMethodArgumentSelector<> {
        using list = core::common::types::TypeList<>;
    };

    template <typename TFirst, typename... TRest>
    struct RuntimeSystemMainMethodArgumentSelector<TFirst, TRest...> {

        using FirstType = std::remove_cvref_t<TFirst>;
        using Rest = RuntimeSystemMainMethodArgumentSelector<TRest...>;

        static constexpr bool IsQuery = entity::concepts::IsQuery<FirstType>;

        using list = std::conditional_t<
                IsQuery,
                typename Rest::list,
                // do not append remove_cvref_t since type is required.
                typename Rest::list::template Prepend<TFirst>
        >;
    };

    template <typename>
    struct RuntimeSystemMainMethodArguments;

    template <typename... TArgs>
    struct RuntimeSystemMainMethodArguments<std::tuple<TArgs...>> : RuntimeSystemMainMethodArgumentSelector<TArgs...> {};

}