/**
 * @file IsManagerLike.ixx
 * @brief Concept constraining types eligible for manager registration.
 */
module;

#include <type_traits>

export module helios.ecs.manager.concepts:IsManagerLike;

export namespace helios::ecs::manager::concepts {

template <class TManager>
concept IsManagerLike = requires {
    &std::remove_cvref_t<TManager>::commit;
    &std::remove_cvref_t<TManager>::reset;
};
} // namespace helios::ecs::manager::concepts
