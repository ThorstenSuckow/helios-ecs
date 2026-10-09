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

export module helios.ecs.scheduling.Scheduler:Schedule;

import helios.core.common;
import helios.core.thread.ThreadPool;

import helios.ecs.scheduling.traits;
import helios.ecs.scheduling.concepts;


import helios.ecs.component.components;

import helios.ecs.entity;
import helios.ecs.common;
import helios.ecs.manager;
import helios.ecs.system;
import helios.ecs.command;

using namespace helios::ecs::common::types;
using namespace helios::ecs::common::concepts;

export namespace helios::ecs::scheduling {

    class Scheduler;


    class Schedule {

        using EcsDataContainer = ecs::common::container::EcsDataContainer;
        using ThreadPool = helios::core::thread::ThreadPool;


        ecs::system::SystemRegistry systemRegistry_{};

        std::vector<std::vector<std::vector<ecs::system::types::SystemTypeId>>> systemTypeIdQueue_;

        std::vector<bool> registeredManagerTypeIds_;
        std::vector<std::vector<std::vector<ecs::manager::types::ManagerTypeId>>> managerTypeIdQueue_;

        template<typename TManager>
        void registerManager() {
            auto typeId = manager::types::ManagerTypeId::template id<TManager>();
            auto* reg = ecsDataContainer_.tryGet<ecs::manager::ManagerRegistry>();
            #if HELIOS_DEBUG
            if (!reg) {
                assert(reg && "ManagerRegistry not found in EcsDataContainer");
            }
            #endif

            if (registeredManagerTypeIds_.size() <= typeId.value()) {
                registeredManagerTypeIds_.resize(typeId.value() + 1);
            }
            if (registeredManagerTypeIds_[typeId.value()]) {
                assert(false && "Manager was already registered with this schedule.");
                std::terminate();
            }
            registeredManagerTypeIds_[typeId.value()] = true;

            auto* manager = reg->item(typeId);
            if (!manager) {
                 if constexpr (std::is_default_constructible_v<TManager>) {
                    reg->add<TManager>(TManager{});
                } else {
                    assert(false && "Failed to construct manager");
                    std::terminate();
                }
            }
        }



        template<typename TSystem>
        Schedule& registerCallOperatorSystem(TSystem&& system) {
            using SystemType = std::remove_cvref_t<TSystem>;
            systemRegistry_.template add<SystemType>(ecs::system::System(std::move(system)));
            return *this;
        }

        template<typename TSystem>
        requires ecs::system::concepts::IsTypedSystem<std::remove_cvref_t<TSystem>>
        Schedule& registerTypedSystemInstance(TSystem&& system) {
            using SystemType = std::remove_cvref_t<TSystem>;
            systemRegistry_.add<SystemType>(std::move(system));
            return *this;
        }

        EcsDataContainer ecsDataContainer_{};

        Scheduler& owner_;

        template<typename TSystem>
        void ensureRequiredStorage(EcsDataContainer& ecsDataContainer) {

            using SystemType = std::remove_cvref_t<TSystem>;

            auto ensureStorage = [&]<typename TAccessSets>() {
                core::common::traits::Apply<typename TAccessSets::list>::forEach(
                [&]<typename TAccessSet> () {

                    using HandleType = typename TAccessSet::HandleType;
                    using ReadSet = typename TAccessSet::ReadSet::list;
                    using WriteSet = typename TAccessSet::WriteSet::list;

                    auto& em = ecsDataContainer.get<ecs::entity::EntityManager<HandleType>>();
                    auto ensure =[&em]<typename TComponent>() {
                        em.template ensureSparseSet<TComponent>();
                    };
                    core::common::traits::Apply<ReadSet>::forEach(ensure);
                    core::common::traits::Apply<WriteSet>::forEach(ensure);
                });
            };

            using ReturnType = traits::RuntimeSystemToMainMethodSignature<SystemType>::ReturnType;
            if constexpr (!std::is_void_v<ReturnType>) {
                auto* resultRegistry = ecsDataContainer.tryGet<ecs::common::RuntimeResultRegistry>();
                assert(resultRegistry && "RuntimeResultRegistry not found in EcsDataContainer");
                resultRegistry->reserve<ReturnType>();
            }

            using AccessSets = traits::RuntimeSystemAccessSets<SystemType>;
            ensureStorage.template operator()<AccessSets>();

        }

        using RunCondition = std::function<bool(EcsDataContainer&)>;
        RunCondition runCondition_;



    public:

        Schedule(const Schedule&) = delete;
        Schedule& operator=(const Schedule&) = delete;
        Schedule(Schedule&&) noexcept = delete;
        Schedule& operator=(Schedule&&) noexcept = delete;



        template<typename TPredicate>
         explicit Schedule(
             Scheduler& owner,
             EcsDataContainer& parentEcsDataContainer,
             TPredicate predicate
         )
         : owner_(owner),
           runCondition_([predicate](auto& ecsDataContainer) -> bool {
               return common::container::EcsDataContainerFunctionInvoker::invoke<&TPredicate::operator()>(
                   predicate, ecsDataContainer
               );
           })

        {ecsDataContainer_.borrow(parentEcsDataContainer);}



        // +---------------------------------
        // | Typed/ Lambda Systems
        // +---------------------------------
        template<typename TSystem>
        requires ecs::system::concepts::IsTypedSystem<TSystem>
        Schedule& add() {
            using SystemType = std::remove_cvref_t<TSystem>;
            return add(SystemType{});
        }

        template<typename TSystem>
        requires ecs::system::concepts::IsTypedSystem<TSystem>
        Schedule& add(TSystem sys) {
            using SystemType = std::remove_cvref_t<TSystem>;
            ensureRequiredStorage<SystemType>(ecsDataContainer_);
            registerTypedSystemInstance<SystemType>(std::move(sys));
            systemTypeIdQueue_.push_back({{ecs::system::types::SystemTypeId::template id<SystemType>()}});
            return *this;
        }

        template<typename TFuncSystem>
        requires ecs::system::concepts::IsCallableSystem<TFuncSystem>
        Schedule& add(TFuncSystem&& system) {
            using SystemType = std::remove_cvref_t<TFuncSystem>;

            ensureRequiredStorage<SystemType>(ecsDataContainer_);

            registerCallOperatorSystem(std::forward<TFuncSystem>(system));
            systemTypeIdQueue_.push_back({{ecs::system::types::SystemTypeId::template id<SystemType>()}});
            return *this;
        }
        // +---------------------------------
        // +---------------------------------


        template<typename ... TSystem>
        requires concepts::ConflictFreeCallableSystems<TSystem...>
        Schedule& add(TSystem&&... system) {

            (ensureRequiredStorage<TSystem>(ecsDataContainer_), ...);

            (registerCallOperatorSystem(std::forward<TSystem>(system)), ...);

            auto& group = systemTypeIdQueue_.emplace_back();
            group.reserve(sizeof...(TSystem));
            (group.push_back({{ecs::system::types::SystemTypeId::template id<std::remove_cvref_t<TSystem>>()}}), ...);
            
            return *this;
        }


        template<typename ...TSystem>
        requires concepts::ConflictFreeTypedSystems<TSystem...>
        Schedule& add() {

            (ensureRequiredStorage<TSystem>(ecsDataContainer_), ...);

            (registerTypedSystemInstance<TSystem>(std::remove_cvref_t<TSystem>{}), ...);

            auto& group = systemTypeIdQueue_.emplace_back();
            group.reserve(sizeof...(TSystem));
            (group.push_back({{ecs::system::types::SystemTypeId::template id<TSystem>()}}), ...);

            return *this;
        }


        template <typename ... TSequentials>
        requires concepts::ConflictFreeSequentialSystems<TSequentials...>
        Schedule& add() {
            return add(std::remove_cvref_t<TSequentials>{}...);
        }


        template <typename ... TSequentials>
        requires concepts::ConflictFreeSequentialSystems<TSequentials...>
        Schedule& add(TSequentials&& ... sequentials) {

            auto& parallelGroup = systemTypeIdQueue_.emplace_back();
            parallelGroup.reserve(sizeof...(TSequentials));

            auto addSystemInstance = [this]<typename TSystem>(auto& sequential, auto& sequentialGroup) {

                ensureRequiredStorage<TSystem>(ecsDataContainer_);

                auto& sys = sequential.template systemFor<TSystem>();

                if constexpr(ecs::system::concepts::IsCallableSystem<TSystem>) {
                    registerCallOperatorSystem(std::forward<TSystem>(sys));
                } else {
                    registerTypedSystemInstance<TSystem>(std::move(sys));
                }
                sequentialGroup.push_back({{ecs::system::types::SystemTypeId::template id<TSystem>()}});
            };

            auto registerSystems = [this, &addSystemInstance]<typename ...TSystems>
            (common::Sequential<TSystems...>& sequential, auto& sequentialGroup) {
                sequentialGroup.reserve(sizeof...(TSystems));
                (addSystemInstance.template operator()<TSystems>(sequential, sequentialGroup), ...);
            };


            (registerSystems(sequentials, parallelGroup.emplace_back()), ...);
            return *this;
        }

        Scheduler& endSchedule() {
            return owner_;
        }

        template<typename... TManagers>
        requires concepts::ConflictFreeManagers<TManagers...>
        Scheduler& endSchedule() {
            endSchedule<common::Sequential<TManagers>...>();
            return owner_;
        }

        template <typename ... TSequentials>
        requires concepts::ConflictFreeSequentialManagers<TSequentials...>
        Scheduler& endSchedule() {
            auto& parallelGroup = managerTypeIdQueue_.emplace_back();
            parallelGroup.reserve(sizeof...(TSequentials));

            auto addManager = [this]<typename TManager>(auto& sequentialGroup) {
                ensureRequiredStorage<TManager>(ecsDataContainer_);
                registerManager<TManager>();
                sequentialGroup.push_back({{ecs::manager::types::ManagerTypeId::template id<TManager>()}});
            };

            auto registerManagers = [this, &addManager]<typename ...TManagers>(
                std::type_identity<common::Sequential<TManagers...>>,
                auto& sequentialGroup
            ) {
                sequentialGroup.reserve(sizeof...(TManagers));
                (addManager.template operator()<TManagers>(sequentialGroup), ...);
            };


            (registerManagers(
                std::type_identity<TSequentials>{},
                parallelGroup.emplace_back()), ...
            );

            return owner_;
        }



        // +---------------------------
        // | Runtime
        // +---------------------------
        void onScheduleEnd() noexcept {

            if (managerTypeIdQueue_.empty()) {
                return;
            }

            auto* reg = ecsDataContainer_.tryGet<ecs::manager::ManagerRegistry>();
            #if HELIOS_DEBUG
            if (!reg) {
                assert(reg && "ManagerRegistry not found in EcsDataContainer");
            }
            #endif

             run(managerTypeIdQueue_, *reg, [](auto& manager, auto& ecsDataContainer) {
                manager.execute(ecsDataContainer);
            });
        }

        void update() {
            run(systemTypeIdQueue_, systemRegistry_, [](auto& system, auto& ecsDataContainer) {
                system.update(ecsDataContainer);
            });
        }

        template<typename TRuntimeQueue, typename TRegistry, typename TExecute>
        void run(const TRuntimeQueue& queue, TRegistry& registry, TExecute cmd) {


            auto invalidateView = [this](bool mustInvalidate) {
                if (mustInvalidate) {
                    if (auto* resultRegistry = ecsDataContainer_.tryGet<ecs::common::RuntimeResultRegistry>()) {
                        assert(resultRegistry && "RuntimeResultRegistry not found in EcsDataContainer");
                        resultRegistry->invalidateView();
                    }
                }
            };

            auto* threadPool = ecsDataContainer_.tryGet<ThreadPool>();
            assert(threadPool && "ThreadPool not found in EcsDataContainer");

            for (auto& parallelRuntimeSystems : queue) {
                 bool mustInvalidate = false;
                // parallelSystems with only one entry are treated serial
                if (parallelRuntimeSystems.size() == 1) {
                    for (const auto& serialRuntimeSystem : parallelRuntimeSystems[0]) {
                        auto* runtimeSystem = registry.item(serialRuntimeSystem);
                        // update, commit mutations
                        cmd(*runtimeSystem, ecsDataContainer_);
                        // produce system results, flush any underlying flushable objects
                        runtimeSystem->flush(ecsDataContainer_);

                        if (runtimeSystem->hasResult()) {
                            mustInvalidate = true;
                        }

                    }
                    invalidateView(mustInvalidate);
                    continue;
                }
                mustInvalidate = false;
                // parallelSystems > 1 will be queued with the ThreadPool
                threadPool->runAndWait(
                    parallelRuntimeSystems.size(),
                    [&] (const std::size_t i) {
                        // a parallel system owns more ore more serial systems
                        for (const auto& serialRuntimeSystem : parallelRuntimeSystems[i]) {
                            auto* runtimeSystem = registry.item(serialRuntimeSystem);
                            cmd(*runtimeSystem, ecsDataContainer_);
                        }
                });

                // once parallel systems where updates, flush their command buffers
                for (const auto& parallelRuntimeSystem : parallelRuntimeSystems) {
                    for (const auto& serialRuntimeSystem : parallelRuntimeSystem) {
                        auto* runtimeSystem = registry.item(serialRuntimeSystem);
                        runtimeSystem->flush(ecsDataContainer_);
                        if (runtimeSystem->hasResult()) {
                            mustInvalidate = true;
                        }
                    }
                }
                invalidateView(mustInvalidate);
            }
        }

        [[nodiscard]] bool shouldRun() noexcept {
            return runCondition_(ecsDataContainer_);
        }



    };

}