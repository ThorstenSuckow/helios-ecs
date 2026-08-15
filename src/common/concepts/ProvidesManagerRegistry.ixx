/**
 * @file ProvidesManagerRegistry.ixx
 * @brief Concept for types exposing a manager registry accessor.
 */
module;


#include <concepts>


export module helios.ecs.common.concepts:ProvidesManagerRegistry;

export namespace helios::ecs::common::concepts {


    template<typename T, typename TManagerRegistry>
    concept ProvidesManagerRegistry = requires(T& t) {
        {t.managerRegistry()} -> std::same_as<TManagerRegistry&>;
    };


}