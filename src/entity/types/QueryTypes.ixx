module;



export module helios.ecs.entity.types:QueryTypes;

export namespace helios::ecs::entity::types {
    template<template <typename> typename... >
    struct Read{};

    template<template <typename> typename... >
    struct Write{};
}
