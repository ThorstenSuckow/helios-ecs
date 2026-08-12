/**
 * @file IsCommandHandlerLike.ixx
 * @brief Concept constraining types that can handle specific commands.
 */
module;

#include <concepts>

export module helios.ecs.command.concepts:IsCommandHandlerLike;

export namespace helios::ecs::command::concepts {

    /**
     * @brief Constrains T to objects that provide a submit method for specified command types.
     *
     * @see CommandHandlerRegistry
     * @see ResourceRegistry
     */
    template<typename OwningT, typename... CommandType>
    concept IsCommandHandlerLike = true;
}
