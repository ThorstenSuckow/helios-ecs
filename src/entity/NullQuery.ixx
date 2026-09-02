/**
 * @file NullQuery.ixx
 * @brief NullQuery as null-type for ecs::entity::Query
 */
module;

export module helios.ecs.entity.NullQuery;

import helios.ecs.entity.EntityAccessSet;

export namespace helios::ecs::entity {

    struct NullQuery {

        using HandleType = void;
        using WriteSet = WriteSet<>;
        using ReadSet = ReadSet<>;
    };

};