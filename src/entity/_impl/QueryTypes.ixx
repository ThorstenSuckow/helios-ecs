/**
 * @file QueryTypes.ixx
 * @brief Query types for reading and writing components.
 */
module;



export module helios.ecs.entity.Query:QueryTypes;

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
}
