/**
 * @file IsEntityManager.ixx
 * @brief Concept for constraining types to EntityManager-like types.
 */
module;

#include <tuple>

export module helios.ecs.entity.concepts:IsEntityManager;



export namespace helios::ecs::entity::concepts {
    
    template<typename T>
    concept IsEntityManager = requires {
        typename std::remove_cvref_t<T>::EntityRegistryType;
        typename std::remove_cvref_t<T>::HandleType;
    };

}