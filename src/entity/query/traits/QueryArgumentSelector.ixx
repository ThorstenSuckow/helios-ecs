/**
* @file QueryArgumentSelector.ixx
 * @brief Trait for determining the type and position of a Query type.
 */
module;

#include <type_traits>
#include <tuple>
#include <variant>

export module helios.ecs.entity.query.traits:QueryArgumentSelector;

import helios.ecs.entity.concepts;
import helios.core.common.traits;
import helios.core.common.types;
import helios.ecs.entity.query.NullQuery;

export namespace helios::ecs::entity::query::traits {

    template <typename... TArgs>
    struct QueryArgumentSelector;

    template <>
    struct QueryArgumentSelector<> {
        using Type = void;
        static constexpr std::size_t Count = 0;
        using list = core::common::types::TypeList<>;
        using handles = core::common::types::TypeList<>;
    };

    template <typename TFirst, typename... TRest>
    struct QueryArgumentSelector<TFirst, TRest...> {

        using FirstType = std::remove_cvref_t<TFirst>;
        using Rest = QueryArgumentSelector<TRest...>;

        static constexpr bool IsQuery = concepts::IsQuery<FirstType>;

        using QueryType = std::conditional_t<IsQuery, FirstType, NullQuery>;
        using handles = typename Rest::handles::template Prepend<
            std::conditional_t<
                IsQuery,
                typename QueryType::HandleType,
                void
        >>;

        using list = typename Rest::list::template Prepend<
            std::conditional_t<
                IsQuery,
                FirstType,
                NullQuery
        >>;
    };

    template <typename>
    struct QueryFromArguments;

    template <typename... TArgs>
    struct QueryFromArguments<std::tuple<TArgs...>> : QueryArgumentSelector<TArgs...> {};

} // namespace helios::ecs::entity::traits