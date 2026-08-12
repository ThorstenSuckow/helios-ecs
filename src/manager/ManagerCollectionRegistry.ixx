/**
 * @file ManagerRegistry.ixx
 * @brief Type-indexed registry for managing Manager instances.
 */
module;

#include <tuple>
#include <vector>

export module helios.ecs.manager.ManagerCollectionRegistry;

import helios.core.container;
import helios.ecs.manager.Manager;

import helios.ecs.manager.ManagerRegistry;
import helios.ecs.manager.types;

export namespace helios::ecs::manager {

    /**
     * @brief Type alias for a ConceptModelRegistry specialized for Managers.
     *
     * @see ConceptModelRegistry
     * @see Manager
     */
    template<typename ... TContextPairs>
    class ManagerCollectionRegistry {

        std::tuple<
            std::vector<
                ManagerRegistry<
                    typename TContextPairs::LeftContext_type,
                    typename TContextPairs::RightContext_type
                >>...> managerRegistries_;

        template<typename TExecutionContext, typename TInitContext>
        [[nodiscard]] ManagerRegistry<TExecutionContext, TInitContext>& getManagerRegistry() {
            return std::get<ManagerRegistry<TExecutionContext, TInitContext>>(managerRegistries_);
        }

        template<typename TManager>
        [[nodiscard]] bool has() const{
            auto& registry = getManagerRegistry<typename TManager::ExecutionContext, typename TManager::InitContext>();
            return registry.template has<TManager>();
        }

        template<typename TManager, typename ... Args>
        [[nodiscard]] TManager& add(Args&& ...args) const{
            auto& registry = getManagerRegistry<typename TManager::ExecutionContext, typename TManager::InitContext>();
            return registry.template add<TManager>(std::forward<Args>(args)...);
        }

        template<typename TManager>
        TManager* tryManager() noexcept {
            auto& registry = getManagerRegistry<typename TManager::ExecutionContext, typename TManager::InitContext>();
            return registry.template tryManager<TManager>();
        }

        template<typename TManager>
        const TManager* tryManager() const noexcept {
            auto& registry = getManagerRegistry<typename TManager::ExecutionContext, typename TManager::InitContext>();
            return registry.template tryManager<TManager>();
        }

    };

}