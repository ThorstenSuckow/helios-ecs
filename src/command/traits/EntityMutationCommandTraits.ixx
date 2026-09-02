/**
 * @file EntityMutationCommandTraits.ixx
 * @brief Traits for EntityMutationCommand.
 */
module;

#include <optional>
#include <vector>

export module helios.ecs.command.traits:EntityMutationCommandTraits;

import helios.core.common.traits;
import helios.core.common.concepts;
import helios.core.common.types;

import helios.ecs.command.EntityMutationCommandSink;
import helios.ecs.command.EntityMutationCommandBuffer;


export namespace helios::ecs::command::traits {

    template<typename THandle, typename TWriteComponents>
    struct EntityMutationCommandSinkFromList;

    template<typename THandle, typename ... TWriteComponents>
    struct EntityMutationCommandSinkFromList<THandle, core::common::types::TypeList<TWriteComponents...>> {

        using type = std::conditional_t<
            sizeof...(TWriteComponents) == 0,
            std::monostate,
            ecs::command::EntityMutationCommandSink<THandle, TWriteComponents...>
        >;

    };

    template<typename TQueries>
    struct EntityMutationCommandSinksFromQueries;;

    template<typename ... TQueries>
    struct EntityMutationCommandSinksFromQueries<core::common::types::TypeList<TQueries...>> {

        using list = core::common::types::TypeList<
            typename EntityMutationCommandSinkFromList<
            typename TQueries::HandleType,
            typename TQueries::WriteSet::ComponentList
            >::type...
        >;
    };

    template<typename THandles>
    struct EntityMutationCommandBuffersFromHandles;

    template<typename ... THandles>
    struct EntityMutationCommandBuffersFromHandles<core::common::types::TypeList<THandles...>> {

        using tuple = std::tuple<
            std::conditional_t<
                std::same_as<THandles, void>,
                std::monostate,
                ecs::command::EntityMutationCommandBuffer<THandles>
        > ...>;
    };



}

