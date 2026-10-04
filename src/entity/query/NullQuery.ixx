/**
 * @file NullQuery.ixx
 * @brief NullQuery as null-type for ecs::entity::query::Query
 */
module;

export module helios.ecs.entity.query.NullQuery;

import helios.ecs.entity.QueryAccessSet;

export namespace helios::ecs::entity::query {

    struct NullQuery {

        using HandleType = void;
        using WriteSet = WriteSet<>;
        using ReadSet = ReadSet<>;
    };

};