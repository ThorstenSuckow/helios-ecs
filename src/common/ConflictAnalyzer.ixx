/**
 * @file ConflictAnalyzer.ixx
 * @brief Compile-time analyzer for Read/Write Sets.
 */
module;

#include <concepts>

export module helios.ecs.common.ConflictAnalyzer;

import helios.core.common.traits;
import helios.core.common.types;

import helios.ecs.entity.EntityAccessSet;


export namespace helios::ecs::common::ConflictAnalyzer {

    template<typename ... TAccessSets>
    struct HasConflict;

    template<typename THandleA, typename TReadSetA, typename TWriteSetA, typename THandleB, typename TReadSetB, typename TWriteSetB>
    struct HasConflict<
        ecs::entity::EntityAccessSet<THandleA, TReadSetA, TWriteSetA>,
        ecs::entity::EntityAccessSet<THandleB, TReadSetB, TWriteSetB>> {

        using ReadSetA = TReadSetA::ComponentList;
        using WriteSetA = TWriteSetA::ComponentList;

        using ReadSetB = TReadSetB::ComponentList;
        using WriteSetB = TWriteSetB::ComponentList;

        using WriteWriteConflict = core::common::traits::IntersectionList<WriteSetA, WriteSetB>::list;
        using ReadWriteConflict = core::common::traits::IntersectionList<ReadSetA, WriteSetB>::list;
        using WriteReadConflict = core::common::traits::IntersectionList<WriteSetA, ReadSetB>::list;

        static constexpr bool value =
            std::same_as<THandleA, THandleB> &&
                (WriteWriteConflict::size > 0 ||
                ReadWriteConflict::size > 0 ||
                WriteReadConflict::size > 0);
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
