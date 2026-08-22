/**
* @file CommandBufferSelector.ixx
 * @brief Trait for determining the type and position of a CommandBufferLike type.
 */
module;


#include <utility>

export module helios.ecs.command.traits:CommandBufferSelector;

import helios.ecs.command.NullCommandBuffer;
import helios.ecs.command.concepts;

export namespace helios::ecs::command::traits {

    template<typename... TArgs>
    struct CommandBufferSelector;

    template<>
    struct CommandBufferSelector<> {
        using Type = NullCommandBuffer;
        static constexpr std::size_t Count = 0;
    };

    template<typename TFirst, typename... TRest>
    struct CommandBufferSelector<TFirst, TRest...> {

        using FirstType = std::remove_cvref_t<TFirst>;
        using Rest = CommandBufferSelector<TRest...>;

        static constexpr bool IsCommandBuffer = command::concepts::IsCommandBufferLike<FirstType>;
        static constexpr std::size_t Count = (IsCommandBuffer ? 1 : 0) + Rest::Count;

        using Type = std::conditional_t<
            IsCommandBuffer,
            FirstType,
            typename Rest::Type
        >;
    };

    template<typename>
    struct CommandBufferFromArguments;

    template<typename... TArgs>
    struct CommandBufferFromArguments<std::tuple<TArgs...>> : CommandBufferSelector<TArgs...> {};

}