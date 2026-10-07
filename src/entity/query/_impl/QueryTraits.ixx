/**
 * @file QueryTraits.ixx
 * @brief Traits for easing access to ecs::entity::query::Query-related information from types.
 */
module;

#include <tuple>


export module helios.ecs.entity.query.Query:QueryTraits;

import :QueryTypes;


import helios.ecs.entity.storage.SparseSet;
import helios.ecs.component.components;

import helios.ecs.entity.EntityManager;
import helios.ecs.entity.QueryAccessSet;

export namespace helios::ecs::entity::query {
template <
    typename TEntityManager,
    typename TReadComponents,
    typename TWriteComponents,
    typename TDirtyComponents,
    typename TOptionalComponents
>
class PartialQuery;
}


export namespace helios::ecs::entity::traits {

    template<
        typename THandle,
        typename TReadSet,
        typename TWriteSet,
        typename TFilter,
        typename TOptionalComponents
   >
   struct QueryBuilder {

        using ReadComponents = std::conditional_t<
           TFilter::onlyActive,
           typename TReadSet::list::template Prepend<ecs::components::Active>,
           typename TReadSet::list
       >;

        using type =  entity::query::PartialQuery<
            entity::EntityManager<THandle>,
            ReadComponents,
            typename TWriteSet::list,
            TFilter,
            typename TOptionalComponents::list
        >;
    };


    template<typename THandle, typename T>
    struct DirtySetTrait;

    template<typename THandle, typename ... TComponents>
    struct DirtySetTrait<THandle, core::common::types::TypeList<TComponents...>> {
        using tuple = std::tuple<ecs::entity::storage::SparseSet<THandle, TComponents>*...>;
        using readSet = ReadSet<TComponents...>;
    };

}

