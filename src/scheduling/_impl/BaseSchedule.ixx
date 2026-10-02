/**
 * @file Pass.ixx
 * @brief Abstract base class for game loop passes.
 */
module;

#include <type_traits>
#include <utility>
#include <memory>
#include <cassert>
#include <vector>
#include <functional>
#include <iostream>
#include <exception>
#include "helios-ecs-config.h"

export module helios.ecs.scheduling.Scheduler:BaseSchedule;

import helios.core.common;
import helios.core.thread.JobSystem;

import helios.ecs.scheduling.traits;
import helios.ecs.scheduling.concepts;

import helios.ecs.common;
import helios.ecs.manager;
import helios.ecs.system;
import helios.ecs.command;

using namespace helios::ecs::common::types;
using namespace helios::ecs::common::concepts;

export namespace helios::ecs::scheduling {

    class Scheduler;

    /**
     * @brief Abstract base class for game loop passes.
     *
     * @details A Pass represents a logical grouping of systems executed
     * sequentially within a Phase. Concrete implementations (TypedPass)
     * add state-based filtering via shouldRun().
     *
     * ## Key Features
     *
     * - **System Registration:** Systems are added via add<T>()
     * - **Commit Points:** Control when events/commands are synchronized
     * - **State Filtering:** Passes can be skipped based on game state
     *
     * @see TypedPass
     * @see Phase
     * @see System
     */
    class BaseSchedule {

        friend class helios::ecs::scheduling::Scheduler;

        using EcsDataContainer = ecs::common::container::EcsDataContainer;
        using JobSystem = helios::core::thread::JobSystem;

    protected:
        /**
         * @brief Registry holding all systems for this pass.
         */
        ecs::system::SystemRegistry systemRegistry_{};

        /**
         * @brief Ordered queue of system type IDs that drives execution order within this pass.
         */
        std::vector<std::vector<std::vector<ecs::system::types::SystemTypeId>>> systemTypeIdQueue_;

        /**
         * @brief List of ManagerTypeIds.
         */
        std::vector<ecs::manager::types::ManagerTypeId> managerTypeIds_;

        /**
         * @brief List of ManagerTypeIds for parallel execution.
         */
        std::vector<ecs::manager::types::ManagerTypeId> parallelManagerTypeIds_;

        /**
         * @brief Registers the ManagerTypeIds for the Managers this pass should flush.
         *
         * @tparam T The type of the manager to register.
         */
        template<typename T>
        void registerManagerExecuteCommands(EcsDataContainer& ecsDataContainer) {

            auto* reg = ecsDataContainer.tryGet<ecs::manager::ManagerRegistry>();
            #if HELIOS_DEBUG
            if (!reg) {
                assert(reg && "ManagerRegistry not found in EcsDataContainer");
            }
            #endif

            if (!reg->item<T>()) {

                if constexpr (std::is_default_constructible_v<T>) {
                    reg->add<T>(T{});
                } else {
                    assert(false && "Failed to construct manager");
                    std::terminate();
                }
            }

            managerTypeIds_.push_back(ecs::manager::types::ManagerTypeId::template id<T>());
        }

        template<typename TSystem>
        BaseSchedule& registerCallOperatorSystem(TSystem&& system) {
            using SystemType = std::remove_cvref_t<TSystem>;
            systemRegistry_.template add<SystemType>(ecs::system::System(std::move(system)));
            return *this;
        }

        /**
         * @brief Registers a system instance for parallel typed systems.
         */
        template<typename TSystem>
        requires ecs::system::concepts::IsTypedSystem<std::remove_cvref_t<TSystem>>
        BaseSchedule& registerTypedSystemInstance(TSystem&& system) {
            using SystemType = std::remove_cvref_t<TSystem>;
            systemRegistry_.add<SystemType>(std::move(system));
            return *this;
        }

        /**
         * @brief Registers a system described by a `TypedSystemSpec` with this pass.
         *
         * @tparam T A `TypedSystemSpec` specialisation whose `System_type` satisfies
         *           `IsTypedSystem`.
         * @param spec Spec instance carrying the system type and its construction arguments.
         * @return Reference to this pass.
         */
        template<typename T>
        BaseSchedule& registerTypedSystemSpec(T&& spec) {

            using Spec = std::remove_cvref_t<T>;
            using TSystem = Spec::System_type;

            std::apply([this](auto&... args) {
                registerTypedSystem<TSystem>(args...);
            }, spec.args);

            return *this;
        }

        /**
         * @brief Called on pass end.
         * @param ecsDataContainer The ECS data container.
         */
        virtual void onScheduleEnd(EcsDataContainer& ecsDataContainer) = 0;

        /**
         * @brief Updates all systems in this pass.
         *
         * @param ecsDataContainer The map of results from the current frame's system executions.
         * @param jobSystem The job system used for parallel execution of systems.
         */
        virtual void update(EcsDataContainer& ecsDataContainer, JobSystem& jobSystem) = 0;


        /**
         * @brief Determines if this pass should execute.
         *
         * @param ecsDataContainer
         *
         * @return True if the pass should run.
         */
        virtual bool shouldRun(EcsDataContainer& ecsDataContainer) const noexcept = 0;

        /**
         * @brief Returns a span of the ManagerTypeIds this pass is flushing.
         *
         * @return A span of ManagerTypeIds.
         */
        [[nodiscard]] std::span<const ecs::manager::types::ManagerTypeId> managerTypeIds() noexcept {
            return managerTypeIds_;
        }

        EcsDataContainer ecsDataContainer_{};

        Scheduler& owner_;

    public:

         using RunCondition = std::function<bool()>;


        virtual ~BaseSchedule() = default;


        explicit BaseSchedule(Scheduler& owner)
        : owner_(owner) {}




        // +---------------------------------
        // | Typed/ Lambda Systems
        // +---------------------------------
        template<typename TSystem>
        requires ecs::system::concepts::IsTypedSystem<TSystem>
        BaseSchedule& add() {
            using SystemType = std::remove_cvref_t<TSystem>;
            registerTypedSystemInstance<SystemType>(SystemType{});
            systemTypeIdQueue_.push_back({{ecs::system::types::SystemTypeId::template id<SystemType>()}});
            return *this;
        }

        template<typename TFuncSystem>
        requires ecs::system::concepts::IsCallableSystem<TFuncSystem>
        BaseSchedule& add(TFuncSystem&& system) {
            using SystemType = std::remove_cvref_t<TFuncSystem>;
            registerCallOperatorSystem(std::forward<TFuncSystem>(system));
            systemTypeIdQueue_.push_back({{ecs::system::types::SystemTypeId::template id<SystemType>()}});
            return *this;
        }
        // +---------------------------------
        // +---------------------------------

        /**
         * @brief Adds multiple systems to this pass, allowing for parallel execution.
         *
         * @tparam TSystem The types of the systems to add.
         * @param system The system instances to add.
         * @return Reference to this pass.
         */
        template<typename ... TSystem>
      //  requires concepts::ConflictFreeCallableSystems<TSystem...>
        BaseSchedule& add(TSystem&&... system) {
            (registerCallOperatorSystem(std::forward<TSystem>(system)), ...);

            auto& group = systemTypeIdQueue_.emplace_back();
            group.reserve(sizeof...(TSystem));
            (group.push_back({{ecs::system::types::SystemTypeId::template id<std::remove_cvref_t<TSystem>>()}}), ...);
            
            return *this;
        }


        /**
         * @brief Adds TypedSystem-like systems that should be executed parallel.
         *
         * @tparam TSystem The types of the systems to add.
         *
         * @return Reference to this Pass for method chaining.
         */
        template<typename ...TSystem>
        requires concepts::ConflictFreeTypedSystems<TSystem...>
        BaseSchedule& add() {

            (registerTypedSystemInstance<TSystem>(std::remove_cvref_t<TSystem>{}), ...);

            auto& group = systemTypeIdQueue_.emplace_back();
            group.reserve(sizeof...(TSystem));
            (group.push_back({{ecs::system::types::SystemTypeId::template id<TSystem>()}}), ...);

            return *this;
        }

        /**
         * @brief Adds one or more `Sequential`-wrapped system groups that may execute in parallel.
         *
         * Each `Sequential<S1, S2, …>` argument defines an ordered sub-group: its member systems
         * are registered and run sequentially relative to each other, while distinct `Sequential`
         * arguments form independent parallel lanes that the scheduler may execute concurrently.
         *
         * Example:
         * ```cpp
         * pass.add<
         *     Sequential<PhysicsUpdate, PhysicsCollision>,
         *     Sequential<AudioUpdate>
         * >();
         * ```
         *
         * @tparam TSequentials One or more `Sequential<…>` specialisations satisfying `IsSequentialLike`.
         *                  At least one type is required.
         * @return Reference to this pass for method chaining.
         */
        template <typename ... TSequentials>
        requires concepts::ConflictFreeSequentialSystems<TSequentials...>
        BaseSchedule& add() {

            auto& parallelGroup = systemTypeIdQueue_.emplace_back();
            parallelGroup.reserve(sizeof...(TSequentials));

            auto addSystemInstance = [this]<typename TSystem>(auto& serialGroup) {
                registerTypedSystemInstance<TSystem>(TSystem{});
                serialGroup.push_back({{ecs::system::types::SystemTypeId::template id<TSystem>()}});
            };

            auto registerSystems = [this, &addSystemInstance]<typename ...TSystems>
            (ecs::system::Sequential<TSystems...>, auto& serialGroup) {
                serialGroup.reserve(sizeof...(TSystems));
                (addSystemInstance.template operator()<TSystems>(serialGroup), ...);
            };

            (registerSystems( std::remove_cvref_t<TSequentials>{}, parallelGroup.emplace_back()), ...);


            return *this;
        }

        /**
         * @brief  Registers the Managers this pass should flush.
         *
         * @tparam T The types of the Managers to flush.
         * @return Reference to this Pass for method chaining.
         */
        template<typename... T>
        requires (ecs::manager::concepts::IsManagerLike<T> && ...)
        Scheduler& endSchedule() {

            (registerManagerExecuteCommands<T>(ecsDataContainer_), ...);

            return owner_;
        }

    };

}