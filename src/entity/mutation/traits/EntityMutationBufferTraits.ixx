/**
 * @file EntityMutationCommandTraits.ixx
 * @brief Traits for EntityMutationCommand.
 */
module;

#include <optional>
#include <vector>

export module helios.ecs.entity.mutation.traits:EntityMutationBufferTraits;

import helios.core.common.traits;
import helios.core.common.concepts;
import helios.core.common.types;

import helios.ecs.entity.mutation.EntityMutationBuffer;



export namespace helios::ecs::entity::mutation::traits {

    template<typename THandle, typename TWriteComponents>
    struct EntityMutationBufferFromList;

    template<typename THandle, typename ... TWriteComponents>
    struct EntityMutationBufferFromList<THandle, core::common::types::TypeList<TWriteComponents...>> {

        using type = std::conditional_t<
            sizeof...(TWriteComponents) == 0,
            std::monostate,
            ecs::entity::mutation::EntityMutationBuffer<THandle, TWriteComponents...>
        >;

    };


    template<typename TQueries>
    struct EntityMutationBufferFromQueries;;

    template<typename ... TQueries>
    struct EntityMutationBufferFromQueries<core::common::types::TypeList<TQueries...>> {

        using list = core::common::types::TypeList<
            typename EntityMutationBufferFromList<
            typename TQueries::HandleType,
            typename TQueries::WriteSet::ComponentList
            >::type...
        >;
    };


}

