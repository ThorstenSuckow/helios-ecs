module;

export module helios.ecs.scheduling.traits:QueriesToAccessSet;

import helios.ecs.entity.EntityAccessSet;
import helios.core.common;

export namespace helios::ecs::scheduling::traits {

    template<typename TQuery>
    struct QueryToAccessSet {

        using type = entity::EntityAccessSet<
            typename TQuery::HandleType,
            typename TQuery::ReadSet,
            typename TQuery::WriteSet
        >;
    };

    template<typename TList>
    struct QueriesToAccessSets;

    template<typename ... TQuery>
    struct QueriesToAccessSets<core::common::types::TypeList<TQuery...>> {
        using list = core::common::types::TypeList<
            typename  QueryToAccessSet<TQuery>::type...
        >;
    };


}