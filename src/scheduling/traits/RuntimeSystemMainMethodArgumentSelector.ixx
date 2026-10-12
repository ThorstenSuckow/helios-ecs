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
import helios.core.thread;
import helios.ecs.command.concepts;

export namespace helios::ecs::scheduling::traits {

    template <typename... TArgs>
    struct RuntimeSystemMainMethodArgumentSelector;

    template <>
    struct RuntimeSystemMainMethodArgumentSelector<> {
        using list = core::common::types::TypeList<>;
    };

    template<typename TList>
    struct RuntimeSystemMainMethodArgumentSelectorFromList;

    template<typename... TArgs>
    struct RuntimeSystemMainMethodArgumentSelectorFromList<
        core::common::types::TypeList<TArgs...>> {
        using list = typename RuntimeSystemMainMethodArgumentSelector<TArgs...>::list;
    };

    template <typename TFirst, typename... TRest>
    struct RuntimeSystemMainMethodArgumentSelector<TFirst, TRest...> {

        using FirstType = std::remove_cvref_t<TFirst>;
        using Rest = RuntimeSystemMainMethodArgumentSelector<TRest...>;

        static constexpr bool IsCommandBuffer = ecs::command::concepts::IsCommandBufferLike<FirstType>;
        static constexpr bool IsThreadPool = std::same_as<FirstType, core::thread::ThreadPool>;

        using list = std::conditional_t<
                IsCommandBuffer || IsThreadPool,
                typename Rest::list,
                // do not append remove_cvref_t since qualifiers are required.
                typename Rest::list::template Prepend<TFirst>
        >;

    };

    template <typename>
    struct RuntimeSystemMainMethodArguments;

    template <typename... TArgs>
    struct RuntimeSystemMainMethodArguments<std::tuple<TArgs...>> : RuntimeSystemMainMethodArgumentSelector<TArgs...> {};

}