/**
 * @file ConflictAnalyzer.ixx
 * @brief Compile-time analyzer for Read/Write Sets.
 */
module;

#include <concepts>
#include <utility>

export module helios.ecs.scheduling.traits:ConflictAnalyzer;

import :SystemAccessSets;
import :SequentialAccessSets;
import :SequentialSystemUpdateMethodSignatureSets;
import :SystemToUpdateMethodSignature;

import helios.core.common.traits;
import helios.core.common.types;

import helios.ecs.system.Sequential;

import helios.ecs.entity.QueryAccessSet;
import helios.ecs.system.types;

namespace {
    template<typename TArgA, typename TList>
    struct HasConflictWithArgument;

    template<typename TListA, typename TListB>
    struct HasArgumentTypeConflict;


    template<typename TLeft, typename TRight>
    struct IsArgumentTypeConflict {
        using TypeA = std::remove_cvref_t<TLeft>;
        using TypeB = std::remove_cvref_t<TRight>;

        static constexpr bool ConstA = std::is_const_v<std::remove_reference_t<TLeft>>;
        static constexpr bool ConstB = std::is_const_v<std::remove_reference_t<TRight>>;

        static constexpr bool RefA = std::is_reference_v<TLeft>;
        static constexpr bool RefB = std::is_reference_v<TRight>;

        static constexpr bool value = std::same_as<TypeA, TypeB> &&
            ((RefA && !ConstA) || (RefB && !ConstB));

    };

    template<typename TArgA, typename ... TRest>
    struct HasConflictWithArgument<TArgA, helios::core::common::types::TypeList<TRest...>> {
        static constexpr bool value = (
            (IsArgumentTypeConflict<TArgA, TRest>::value || ...)
        );
    };

    template<typename ...TArgsA, typename ... TArgsB>
    struct HasArgumentTypeConflict<
        helios::core::common::types::TypeList<TArgsA...>,
        helios::core::common::types::TypeList<TArgsB...>> {
        static constexpr bool value = (
            HasConflictWithArgument<TArgsA, helios::core::common::types::TypeList<TArgsB...>>::value || ...
        );
    };

    template<typename ... TAccessSets>
    struct IsConflictPair;

    template<typename TSignatureA, typename TSignatureB>
    struct IsConflictPair<TSignatureA, TSignatureB> {

        using ReturnTypeA = std::remove_cvref_t<typename TSignatureA::ReturnType>;
        using ReturnTypeB = std::remove_cvref_t<typename TSignatureB::ReturnType>;

        static constexpr bool VoidA = std::is_void_v<ReturnTypeA>;
        static constexpr bool VoidB = std::is_void_v<ReturnTypeB>;

        static constexpr bool value =
            ((!VoidA) && std::same_as<ReturnTypeA, ReturnTypeB>) ||
            (HasArgumentTypeConflict<
                typename TSignatureA::ArgumentTypeList,
                typename TSignatureB::ArgumentTypeList
            >::value) ||
            // check the return types. If at least one return type (treated as ref since it
            // represents a value being written) appears in another systems
            // as a R or W value, this is a conflict.
            (!VoidA && HasConflictWithArgument<
                std::add_lvalue_reference_t<ReturnTypeA>, typename TSignatureB::ArgumentTypeList
                >::value) ||
            (!VoidB && HasConflictWithArgument<
                std::add_lvalue_reference_t<ReturnTypeB>, typename TSignatureA::ArgumentTypeList
            >::value);

    };

    template<typename THandleA, typename TReadSetA, typename TWriteSetA, typename THandleB, typename TReadSetB, typename TWriteSetB>
    struct IsConflictPair<
        helios::ecs::entity::QueryAccessSet<THandleA, TReadSetA, TWriteSetA>,
        helios::ecs::entity::QueryAccessSet<THandleB, TReadSetB, TWriteSetB>> {

        using ReadSetA = TReadSetA::list;
        using WriteSetA = TWriteSetA::list;

        using ReadSetB = TReadSetB::list;
        using WriteSetB = TWriteSetB::list;

        using WriteWriteConflict = helios::core::common::traits::IntersectionList<WriteSetA, WriteSetB>::list;
        using ReadWriteConflict = helios::core::common::traits::IntersectionList<ReadSetA, WriteSetB>::list;
        using WriteReadConflict = helios::core::common::traits::IntersectionList<WriteSetA, ReadSetB>::list;

        static constexpr bool value =
            std::same_as<THandleA, THandleB> &&
                (WriteWriteConflict::size > 0 ||
                ReadWriteConflict::size > 0 ||
                WriteReadConflict::size > 0);
    };


    template<typename TAccessSet, typename TList>
    struct HasConflictWithAny;

    template<typename TAccessSet, typename ... TOtherAccessSet>
    struct HasConflictWithAny<TAccessSet, helios::core::common::types::TypeList<TOtherAccessSet...>> {;
        static constexpr bool value = (IsConflictPair<TAccessSet, TOtherAccessSet>::value || ...);
    };

    template<typename TListA, typename TListB>
    struct HasAnyConflict;

    template<typename ... TAccessSetsA, typename ... TAccessSetsB>
    struct HasAnyConflict<helios::core::common::types::TypeList<TAccessSetsA...>, helios::core::common::types::TypeList<TAccessSetsB...>> {
        static constexpr bool value = (HasConflictWithAny<
            TAccessSetsA,
            helios::core::common::types::TypeList<TAccessSetsB...>
        >::value || ...);
    };


    template<typename TSystemA, typename TSystemB>
    struct SystemsConflict : HasAnyConflict<
        typename helios::ecs::scheduling::traits::SystemAccessSets<TSystemA>::list,
        typename helios::ecs::scheduling::traits::SystemAccessSets<TSystemB>::list
    > {};

    template<typename TSequentialA, typename TSequentialB>
    struct SequentialConflict : HasAnyConflict<
        typename helios::ecs::scheduling::traits::SequentialAccessSets<TSequentialA>::list,
        typename helios::ecs::scheduling::traits::SequentialAccessSets<TSequentialB>::list
    > {};

    template<typename TSequentialA, typename TSequentialB>
    struct SequentialSystemUpdateArgumentsConflict : HasAnyConflict<
        typename helios::ecs::scheduling::traits::SequentialSystemUpdateMethodSignatureSets<TSequentialA>::list,
        typename helios::ecs::scheduling::traits::SequentialSystemUpdateMethodSignatureSets<TSequentialB>::list
    > {};

    template<typename TSystemA, typename TSystemB>
    struct SystemUpdateArgumentsConflict : IsConflictPair<
        helios::ecs::scheduling::traits::SystemToUpdateMethodSignature<TSystemA>,
        helios::ecs::scheduling::traits::SystemToUpdateMethodSignature<TSystemB>
    > {};
}

export namespace helios::ecs::scheduling::traits {

    template<typename TList>
    struct HasConflict;

    // system
    template<>
    struct HasConflict<core::common::types::TypeList<>> : std::false_type{};

    template<typename TSystem, typename ... TRest>
    struct HasConflict<core::common::types::TypeList<TSystem, TRest...>> {
        static constexpr bool value =
            (SystemsConflict<TSystem, TRest>::value || ...) ||
            (SystemUpdateArgumentsConflict<TSystem, TRest>::value || ...) ||
            HasConflict<core::common::types::TypeList<TRest...>>::value;
    };

    //sequential
    template<typename ... TSystems, typename ... TRest>
    struct HasConflict<core::common::types::TypeList<system::Sequential<TSystems...>, TRest...>> {
        using Head = system::Sequential<TSystems...>;

        static constexpr bool value = (
            (SequentialConflict<system::Sequential<TSystems...>, TRest>::value || ...) ||
            (SequentialSystemUpdateArgumentsConflict<Head, TRest>::value || ...) ||
            (HasConflict<core::common::types::TypeList<TRest...>>::value)
        );
    };


}
