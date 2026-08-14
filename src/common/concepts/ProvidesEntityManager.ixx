/**
 * @file ProvidesEntityManager.ixx
 * @brief Concept for types exposing an entity manager accessor.
 */
module;


#include <concepts>


export module helios.ecs.common.concepts:ProvidesEntityManager;

export namespace helios::ecs::common::concepts {


    template<
        typename T,
        typename TEntityManager
    >
    concept ProvidesEntityManager = requires(T& t) {
        typename TEntityManager::HandleType;
        {t.template entityManager<typename TEntityManager::HandleType>()} -> std::same_as<TEntityManager&>;
    };


}