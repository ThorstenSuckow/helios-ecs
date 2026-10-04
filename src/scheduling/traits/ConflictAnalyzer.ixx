/**
 * @file ConflictAnalyzer.ixx
 * @brief Compile-time analyzer for Read/Write Sets.
 */
module;

#include <concepts>

export module helios.ecs.scheduling.traits:ConflictAnalyzer;

import :SystemAccessSets;
import :SequentialAccessSets;

import helios.core.common.traits;
import helios.core.common.types;

import helios.ecs.system.Sequential;

import helios.ecs.entity.QueryAccessSet;


export namespace helios::ecs::scheduling::traits {

    template<typename ... TAccessSets>
    struct IsConflictPair;

    template<typename THandleA, typename TReadSetA, typename TWriteSetA, typename THandleB, typename TReadSetB, typename TWriteSetB>
    struct IsConflictPair<
        ecs::entity::QueryAccessSet<THandleA, TReadSetA, TWriteSetA>,
        ecs::entity::QueryAccessSet<THandleB, TReadSetB, TWriteSetB>> {

        using ReadSetA = TReadSetA::list;
        using WriteSetA = TWriteSetA::list;

        using ReadSetB = TReadSetB::list;
        using WriteSetB = TWriteSetB::list;

        using WriteWriteConflict = core::common::traits::IntersectionList<WriteSetA, WriteSetB>::list;
        using ReadWriteConflict = core::common::traits::IntersectionList<ReadSetA, WriteSetB>::list;
        using WriteReadConflict = core::common::traits::IntersectionList<WriteSetA, ReadSetB>::list;

        static constexpr bool value =
            std::same_as<THandleA, THandleB> &&
                (WriteWriteConflict::size > 0 ||
                ReadWriteConflict::size > 0 ||
                WriteReadConflict::size > 0);
    };


    template<typename TAccessSet, typename TList>
    struct HasConflictWithAny;

    template<typename TAccessSet, typename ... TOtherAccessSet>
    struct HasConflictWithAny<TAccessSet, core::common::types::TypeList<TOtherAccessSet...>> {;
        static constexpr bool value =
            (IsConflictPair<TAccessSet, TOtherAccessSet>::value || ...);
    };

    template<typename TListA, typename TListB>
    struct HasAnyConflict;

    template<typename ... TAccessSetsA, typename ... TAccessSetsB>
    struct HasAnyConflict<core::common::types::TypeList<TAccessSetsA...>, core::common::types::TypeList<TAccessSetsB...>> {

        static constexpr bool value =
            (HasConflictWithAny<
                TAccessSetsA,
                core::common::types::TypeList<TAccessSetsB...>
            >::value || ...);
    };

    template<typename TSystemA, typename TSystemB>
    struct SystemsConflict : HasAnyConflict<
        typename SystemAccessSets<TSystemA>::list,
        typename SystemAccessSets<TSystemB>::list
    > {};

    template<typename TSequentialA, typename TSequentialB>
    struct SequentialConflict : HasAnyConflict<
        typename SequentialAccessSets<TSequentialA>::list,
        typename SequentialAccessSets<TSequentialB>::list
    > {};


    template<typename TList>
    struct HasConflict;

    // system
    template<>
    struct HasConflict<core::common::types::TypeList<>> : std::false_type{};

    template<typename TSystem, typename ... TRest>
    struct HasConflict<core::common::types::TypeList<TSystem, TRest...>> {
        static constexpr bool value = (SystemsConflict<TSystem, TRest>::value || ...)
            || HasConflict<core::common::types::TypeList<TRest...>>::value;
    };

    //sequential
    template<typename ... TSystems, typename ... TRest>
    struct HasConflict<core::common::types::TypeList<system::Sequential<TSystems...>, TRest...>> {
        using Head = system::Sequential<TSystems...>;

        static constexpr bool value = (
            SequentialConflict<
                system::Sequential<TSystems...>, TRest
            >::value || ...
        ) || HasConflict<core::common::types::TypeList<TRest...>>::value;
    };


}
