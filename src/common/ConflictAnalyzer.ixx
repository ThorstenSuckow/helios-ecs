/**
 * @file ConflictAnalyzer.ixx
 * @brief Compile-time analyzer for Read/Write Sets.
 */
module;

export module helios.ecs.common.ConflictAnalyzer;

import helios.core.common.traits;
import helios.core.common.types;

import helios.ecs.entity.EntityAccessSet;


export namespace helios::ecs::common::ConflictAnalyzer {

    template<typename ... TAccessSets>
    struct HasConflict;

    template<typename TReadSetA, typename TWriteSetA, typename TReadSetB, typename TWriteSetB>
    struct HasConflict<ecs::entity::EntityAccessSet<TReadSetA, TWriteSetA>, ecs::entity::EntityAccessSet<TReadSetB, TWriteSetB>> {

        using ReadSetA = TReadSetA::ComponentList;
        using WriteSetA = TWriteSetA::ComponentList;

        using ReadSetB = TReadSetB::ComponentList;
        using WriteSetB = TWriteSetB::ComponentList;

        using WriteWrite = core::common::traits::IntersectionList<WriteSetA, WriteSetB>::List;
        using ReadWrite = core::common::traits::IntersectionList<ReadSetA, WriteSetB>::List;
        using WriteRead = core::common::traits::IntersectionList<WriteSetA, ReadSetB>::List;

        static constexpr bool value =
            WriteWrite::size > 0 || ReadWrite::size > 0 || WriteRead::size > 0;
    };


    template<typename TAccessSet>
    struct HasConflict<TAccessSet, core::common::types::TypeList<>> {
        static constexpr bool value = false;
    };

    template<>
    struct HasConflict<core::common::types::TypeList<>> {
        static constexpr bool value = false;
    };

    // [A B C D] -> A [B C D] || [B C D]
    template<typename THead, typename ... TTail>
    struct HasConflict<core::common::types::TypeList<THead, TTail...>> {
        static constexpr bool value =
            HasConflict<THead, core::common::types::TypeList<TTail...>>::value ||
            HasConflict<core::common::types::TypeList<TTail...>>::value;
    };

    // A [B C D] -> A == B ||  A [C D]
    template<typename THead, typename TTail, typename ... TRest>
        struct HasConflict<THead, core::common::types::TypeList<TTail, TRest...>> {
        static constexpr bool value =
            HasConflict<THead, TTail>::value ||
            HasConflict<THead, core::common::types::TypeList<TRest...>>::value;
    };




}
