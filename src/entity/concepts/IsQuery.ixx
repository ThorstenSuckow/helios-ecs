/**
 * @file IsQueryLike.ixx
 * @brief Concept for constraining types to Query-like types.
 */
module;

#include <tuple>

export module helios.ecs.entity.concepts:IsQuery;



export namespace helios::ecs::entity::concepts {


template<typename T>
concept IsQuery = requires {
    typename std::remove_cvref_t<T>::HandleType;
    typename std::remove_cvref_t<T>::ReadSet;
    typename std::remove_cvref_t<T>::WriteSet;
};



}