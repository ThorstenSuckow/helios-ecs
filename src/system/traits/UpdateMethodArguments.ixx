/**
 * @file UpdateMethodArguments.ixx
 * @brief Trait for determining the types and positions of update() arguments, excluding Queries..
 */
module;

#include <type_traits>
#include <tuple>


export module helios.ecs.system.traits:UpdateMethodArguments;

import helios.ecs.entity.concepts;
import helios.core.common.traits;
import helios.core.common.types;
import helios.ecs.entity.query.NullQuery;

export namespace helios::ecs::system::traits {

    template <typename... TArgs>
    struct UpdateMethodArgumentSelector;

    template <>
    struct UpdateMethodArgumentSelector<> {
        using list = core::common::types::TypeList<>;
    };

    template <typename TFirst, typename... TRest>
    struct UpdateMethodArgumentSelector<TFirst, TRest...> {

        using FirstType = std::remove_cvref_t<TFirst>;
        using Rest = UpdateMethodArgumentSelector<TRest...>;

        static constexpr bool IsQuery = entity::concepts::IsQuery<FirstType>;

        using list = std::conditional_t<
                IsQuery,
                typename Rest::list,
                // do not append remove_cvref_t since type is required.
                typename Rest::list::template Prepend<TFirst>
        >;
    };

    template <typename>
    struct UpdateMethodArguments;

    template <typename... TArgs>
    struct UpdateMethodArguments<std::tuple<TArgs...>> : UpdateMethodArgumentSelector<TArgs...> {};

}