/**
 * @file IsManagerLike.ixx
 * @brief Concept constraining types eligible for manager registration.
 */
module;

#include <concepts>

export module helios.ecs.manager.concepts:IsManagerLike;

import helios.ecs.manager.tags;

export namespace helios::ecs::manager::concepts {

    /**
     * @brief Constrains T to types that provide `flush(UpdateContext&)` and
     *        declare `EcsRoleTag = ManagerRole`.
     *
     * @tparam TManager The manager type to constrain.
     */
    template<class TManager>
    concept IsManagerLike = requires (
        TManager& manager,
        typename TManager::InitContextType& initCtx,
        typename TManager::ExecutionContextType& execCtx)
    {
        std::same_as<typename TManager::EcsRoleTag, tags::ManagerRole>;
        {manager.init(initCtx)} -> std::same_as<bool>;
        {manager.executeCommands(execCtx)} -> std::same_as<bool>;
        {manager.reset()} -> std::same_as<void>;
    };
}
