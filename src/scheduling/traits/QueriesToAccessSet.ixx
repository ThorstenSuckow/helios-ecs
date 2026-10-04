module;

export module helios.ecs.scheduling.traits:QueriesToAccessSet;

import helios.ecs.entity.query.NullQuery;

import helios.ecs.entity.QueryAccessSet;
import helios.core.common;


export namespace helios::ecs::scheduling::traits {

    template<typename TQuery>
    struct QueryToAccessSet {
        using list = core::common::types::TypeList<
            entity::QueryAccessSet<
                typename TQuery::HandleType,
                typename TQuery::ReadSet,
                typename TQuery::WriteSet
            >
        >;
    };

    template<>
    struct QueryToAccessSet<entity::query::NullQuery> {
        using list = core::common::types::TypeList<>;
    };

    template<typename TList>
    struct QueriesToAccessSets;

    template<typename... TQuery>
    struct QueriesToAccessSets<core::common::types::TypeList<TQuery...>> {
        using list = typename core::common::traits::ConcatList<
            typename QueryToAccessSet<TQuery>::list...
        >::list;
    };


}