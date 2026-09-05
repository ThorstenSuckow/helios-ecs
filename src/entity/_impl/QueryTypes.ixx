/**
 * @file QueryTypes.ixx
 * @brief Query types for reading and writing components.
 */
module;

export module helios.ecs.entity.Query:QueryTypes;

import helios.core.common.types;

export namespace helios::ecs::entity {
    /**
     * @brief Allows for shorthanding Queries via Query<Handle, Read<Component>, White<Component>>.>
     * for components that are parametrized only by a handle.
     * @tparam T
     */
    template<template <typename> typename... T>
    struct Read{};

    template<template <typename> typename... T>
    struct Write{};

    struct IsActive{};

    struct IsInactive{};

    template<typename ... TDirty>
    struct AnyDirty {
        using list = core::common::types::TypeList<TDirty...>;
    };

    template<typename ... TFilters>
    struct Filter{};

    template<typename ... TDirty>
    struct Filter<AnyDirty<TDirty...>> {
        static constexpr bool onlyActive = false;
        using dirtyList = core::common::types::TypeList<TDirty...>;
    };

    template<>
    struct Filter<IsActive> {
        static constexpr bool onlyActive = true;
        using dirtyList = core::common::types::TypeList<>;
    };


    template<typename ... TDirty>
    struct Filter<IsActive, AnyDirty<TDirty...>> {
        static constexpr bool onlyActive = true;
        using dirtyList = core::common::types::TypeList<TDirty...>;
    };
}
