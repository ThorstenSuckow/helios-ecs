module;

#include <tuple>


export module helios.ecs.entity.Query:QueryTraits;

import helios.ecs.entity.types;

import helios.ecs.entity.EntityManager;
import helios.ecs.entity.EntityAccessSet;

export namespace helios::ecs::entity {
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
    template<typename ... TArgs>
    struct QuerySelector{};

    template<
        typename TReadSet,
        typename TWriteSet
    >
    requires (std::tuple_size_v<typename entity::EntityAccessSet<TReadSet, TWriteSet>::AccessHandles> == 1)
    struct QuerySelector<TReadSet, TWriteSet> {
        using type =  entity::PartialQuery<
            entity::EntityManager<
                    std::tuple_element_t<
                    0,
                    typename ecs::entity::EntityAccessSet<TReadSet, TWriteSet>::AccessHandles
                    >
            >,
        typename TReadSet::ComponentList,
        typename TWriteSet::ComponentList,
        std::tuple<>, std::tuple<>>;
    };


    template<typename THandle, template <typename> typename... TReadComponents, template <typename> typename... TWriteComponents>
    struct QuerySelector<THandle, types::Read<TReadComponents...>, types::Write<TWriteComponents...>>{
        using type = PartialQuery<
                EntityManager<THandle>,
                typename Read<TReadComponents<THandle>...>::ComponentList,
                typename Write<TWriteComponents<THandle>...>::ComponentList,
                std::tuple<>, std::tuple<>
            >;
    };
}

