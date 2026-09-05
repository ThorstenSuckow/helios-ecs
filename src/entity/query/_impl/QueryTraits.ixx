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
import helios.ecs.entity.EntityAccessSet;

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

    template<typename TReadSet, typename TWriteSet>
    using QueryHandle = std::tuple_element_t<0, typename ecs::entity::EntityAccessSet<TReadSet, TWriteSet>::AccessHandles>;

    template<
        typename THandle,
        typename TReadSet,
        typename TWriteSet,
        typename TFilter
   >
   requires (std::tuple_size_v<typename entity::EntityAccessSet<TReadSet, TWriteSet>::AccessHandles> == 1)
   && (std::same_as<THandle, QueryHandle<TReadSet, TWriteSet>>)
   struct QueryBuilderImpl {

        using ReadComponents = std::conditional_t<
           TFilter::onlyActive,
           typename TReadSet::ComponentList::template Prepend<ecs::components::Active<THandle>>,
           typename TReadSet::ComponentList
       >;

        using type =  entity::query::PartialQuery<
            entity::EntityManager<THandle>,
            ReadComponents,
            typename TWriteSet::ComponentList,
            TFilter,
            std::tuple<>
        >;
    };

    template<typename ... T>
    struct QueryBuilder;

    template<
        typename TReadSet,
        typename TWriteSet,
        typename TFilter
   >
   struct QueryBuilder<TReadSet, TWriteSet, TFilter> : QueryBuilderImpl<
        QueryHandle<TReadSet, TWriteSet>, TReadSet, TWriteSet, TFilter
    > {};


    template<typename T>
    struct DirtySetTrait;

    template<typename ... TComponents>
    struct DirtySetTrait<core::common::types::TypeList<TComponents...>> {
        using tuple = std::tuple<ecs::entity::storage::SparseSet<ecs::components::DirtyComponentSpec<TComponents>>*...>;
        using readSet = ReadSet<TComponents...>;
    };

}

