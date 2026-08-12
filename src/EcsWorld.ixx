/**
 * @file GameWorld.ixx
 * @brief Central game state container for entities, resources, and the active level.
 */
module;

#include <cassert>
#include <helios-ecs-config.h>
#include <span>

export module helios.ecs.EcsWorld;

import helios.ecs.Entity;

import helios.ecs.command.CommandHandlerRegistry;
import helios.ecs.command.concepts;

import helios.ecs.manager.ManagerCollectionRegistry;
import helios.ecs.manager.ManagerRegistry;
import helios.ecs.TypedHandleWorld;

import helios.ecs.manager.Manager;
import helios.ecs.manager.types;
import helios.ecs.manager.concepts;
import helios.ecs.manager.ManagerCollectionRegistry;


import helios.ecs.View;

import helios.ecs.concepts;
import helios.ecs.types;

export namespace helios::ecs {

    template<
        typename TTypedHandleWorld,
        typename TManagerCollectionRegistry = manager::ManagerCollectionRegistry<
            types::ContextPair<manager::types::NullExecutionContext, types::NullInitContext
        >>
    >
    class EcsWorld {



    protected:

        command::CommandHandlerRegistry commandHandlerRegistry_;

        TManagerCollectionRegistry managerRegistry_;

        TTypedHandleWorld typedHandleWorld_;

    public:

        EcsWorld() = default;

        /**
         * @brief Non-copyable, non-movable.
         */
        EcsWorld(const EcsWorld&) = delete;
        EcsWorld operator=(const EcsWorld&) = delete;
        EcsWorld(const EcsWorld&&) = delete;
        EcsWorld operator=(const EcsWorld&&) = delete;


        template<typename TInitContext>
        EcsWorld& init(TInitContext& initContext) {
            managerRegistry_.init(initContext);

            return *this;
        }


        /**
         * @brief Checks whether a Manager of type T is registered.
         *
         * @tparam T The Manager type. Must satisfy IsManagerLike.
         *
         * @return True if the Manager is registered.
         */
        template<typename T>
        requires manager::concepts::IsManagerLike<T>
        [[nodiscard]] bool hasManager() const {
            return managerRegistry_.template has<T>();
        }


        /**
         * @brief Registers and constructs a Manager of type T.
         *
         * @tparam T The Manager type. Must satisfy IsManagerLike.
         * @tparam Args Constructor argument types.
         *
         * @param args Arguments forwarded to the T constructor.
         *
         * @return Reference to the newly registered Manager.
         */
        template<typename T, typename... Args>
        requires manager::concepts::IsManagerLike<T>
        T& registerManager(Args&&... args) {
            return managerRegistry_.template add<T>(std::forward<Args>(args)...);
        }

        /**
         * @brief Retrieves a registered Manager by type, or nullptr if not found.
         *
         * @tparam T The Manager type. Must satisfy IsManagerLike.
         *
         * @return Pointer to the Manager, or nullptr if not registered.
         */
        template<typename T>
        requires manager::concepts::IsManagerLike<T>
        T* tryManager() noexcept {
            return managerRegistry_.template item<T>();
        }

        /**
         * @brief Retrieves a registered Manager by type, or nullptr if not found.
         *
         * @tparam T The Manager type. Must satisfy IsManagerLike.
         *
         * @return Const Pointer to the Manager, or nullptr if not registered.
         */
        template<typename T>
        requires manager::concepts::IsManagerLike<T>
        const T* tryManager() const noexcept {
            return managerRegistry_.template item<T>();
        }


        /**
         * @brief Registers a command handler for one or more command types.
         *
         * @tparam CommandType The command types to register handlers for.
         * @tparam OwningT The handler type. Must satisfy IsCommandHandlerLike.
         *
         * @param owner Reference to the handler instance. Must outlive the GameWorld.
         *
         * @see CommandHandlerRegistry
         */
        template<typename... CommandType, typename OwningT>
        requires command::concepts::IsCommandHandlerLike<OwningT, CommandType...>
        void registerCommandHandler(OwningT& owner) {
            (commandHandlerRegistry_.template registerHandler<CommandType>(owner), ...);
        }

        /**
         * @brief Returns a reference to the CommandHandlerRegistry.
         *
         * @return Reference to the CommandHandlerRegistry.
         */
        [[nodiscard]] command::CommandHandlerRegistry& commandHandlerRegistry() noexcept {
            return commandHandlerRegistry_;
        }


        /**
         * @brief Resets all managers and the session to their initial state.
         *
         * @details Called during level transitions or game restarts to clear
         * accumulated state. Invokes reset() on all managers and the session.
         */
        void reset() {
            for (auto& mgr : managerRegistry_.items()) {
                mgr->reset();
            }
        }



        /**
         * @brief Builds a typed ECS view for a handle domain and component set.
         *
         * @tparam THandle Handle domain type.
         * @tparam Components Component types to include.
         *
         * @return Domain-specific view.
         */
        template <typename THandle, typename... Components>
        [[nodiscard]] auto view() {
            return typedHandleWorld_->template view<THandle, Components...>();
        }

        /**
         * @brief Finds an entity facade by handle.
         *
         * @tparam THandle Handle type.
         *
         * @param handle Entity handle to resolve.
         *
         * @return Domain-specific entity facade (or empty facade if not found).
         */
        template<typename THandle>
        [[nodiscard]] auto find(const THandle handle) noexcept {
            return typedHandleWorld_->template find<THandle>(handle);
        }

        /**
         * @brief Adds a new entity in the domain inferred from `THandle`.
         *
         * @tparam THandle Handle type.

         *
         * @return Domain-specific entity facade for the created entity.
         */
        template<typename THandle>
        [[nodiscard]] auto add(const bool isActive = true) noexcept {
            auto entity = typedHandleWorld_->template add<THandle>();
            entity.setActive(isActive);
            return entity;
        }

        /**
         * @brief Destroys an entity in the domain inferred from `THandle`.
         *
         * @tparam THandle Handle type.
         *
         * @param handle Entity handle to destroy.
         *
         * @return Domain-specific destroy result.
         */
        template<typename THandle>
        [[nodiscard]] auto destroy(const THandle handle) noexcept {
            return typedHandleWorld_->template destroy<THandle>(handle);
        }


        /**
         * @brief Returns direct access to the entity manager for a specific handle type.
         *
         * @tparam THandle Handle type.
         * @return Reference to the internal EntityManager for the specified handle type.
         */
        template<typename THandle>
        auto& entityManager() noexcept {
            return typedHandleWorld_->template entityManager<THandle>();
        }

    };

}

