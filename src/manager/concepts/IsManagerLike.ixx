/**
 * @file IsManagerLike.ixx
 * @brief Concept constraining types eligible for manager registration.
 */
module;

#include <utility>

export module helios.ecs.manager.concepts:IsManagerLike;


export namespace helios::ecs::manager::concepts {

    template<class TManager>
    concept IsManagerLike = requires
    {
        &std::remove_cvref_t<TManager>::executeCommands;
        &std::remove_cvref_t<TManager>::reset;
    };
}
