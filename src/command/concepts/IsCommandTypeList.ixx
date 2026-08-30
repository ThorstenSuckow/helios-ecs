module;

#include <type_traits>

export module helios.ecs.command.concepts:IsCommandTypeList;

import helios.ecs.command.types;

export namespace helios::ecs::command::concepts {

template <typename TCommandTypeList>
struct IsCommandTypeList : std::false_type {};

template <typename... TCommands>
struct IsCommandTypeList<ecs::command::types::CommandTypeList<TCommands...>> : std::true_type {};

}; // namespace helios::ecs::command::concepts
