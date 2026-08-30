/**
 * @file ProvidesCommandHandlerRegistry.ixx
 * @brief Concept for types exposing a command handler registry accessor.
 */
module;

#include <concepts>

export module helios.ecs.common.concepts:ProvidesCommandHandlerRegistry;

export namespace helios::ecs::common::concepts {

template <typename T, typename TCommandHandlerRegistry>
concept ProvidesCommandHandlerRegistry = requires(T& t) {
    { t.commandHandlerRegistry() } -> std::same_as<TCommandHandlerRegistry&>;
};

} // namespace helios::ecs::common::concepts