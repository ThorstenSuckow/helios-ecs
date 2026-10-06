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

        std::vector<ecs::manager::types::ManagerTypeId> managerTypeIds_;

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


        [[nodiscard]] std::span<const ecs::manager::types::ManagerTypeId> managerTypeIds() noexcept {
            return managerTypeIds_;
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

            using AccessSets = traits::SystemAccessSets<SystemType>;
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
            (ecs::system::Sequential<TSystems...>& sequential, auto& sequentialGroup) {
                sequentialGroup.reserve(sizeof...(TSystems));
                (addSystemInstance.template operator()<TSystems>(sequential, sequentialGroup), ...);
            };


            (registerSystems(sequentials, parallelGroup.emplace_back()), ...);
            return *this;
        }


        template<typename... T>
        requires (ecs::manager::concepts::IsManagerLike<T> && ...)
        Scheduler& endSchedule() {

            (registerManagerExecuteCommands<T>(ecsDataContainer_), ...);

            return owner_;
        }

        // +---------------------------
        // | Runtime
        // +---------------------------
        void onScheduleEnd(EcsDataContainer& ecsDataContainer) noexcept {

            if (managerTypeIds_.empty()) {
                return;
            }

            auto* reg = ecsDataContainer.tryGet<ecs::manager::ManagerRegistry>();
            #if HELIOS_DEBUG
            if (!reg) {
                assert(reg && "ManagerRegistry not found in EcsDataContainer");
            }
            #endif
            for (const auto typeId : managerTypeIds_) {
                auto* manager = reg->item(typeId);
            #if HELIOS_DEBUG
                if (!manager) {
                    assert(manager && "Manager not found in registry");
                }
            #endif

                manager->execute(ecsDataContainer);
                manager->flush(ecsDataContainer);
            }
        }

        void update(EcsDataContainer& ecsDataContainer) {

            auto* threadPool = ecsDataContainer.tryGet<ThreadPool>();
            assert(threadPool && "ThreadPool not found in EcsDataContainer");

            for (auto& parallelSystems : systemTypeIdQueue_) {

                // parallelSystems with only one entry are treated serial
                if (parallelSystems.size() == 1) {
                    for (const auto& serialSystem : parallelSystems[0]) {
                        auto* system = systemRegistry_.item(serialSystem);
                        // update, commit mutations
                        system->update(ecsDataContainer);
                        // produce system results, flush any underlying flushable objects
                        system->flush(ecsDataContainer);
                    }
                    continue;
                }

                // parallelSystems > 1 will be queued with the ThreadPool
                threadPool->runAndWait(
                    parallelSystems.size(),
                    [&] (const std::size_t i) {
                        // a parallel system owns more ore more serial systems
                        for (const auto& serialSystem : parallelSystems[i]) {
                            auto* system = systemRegistry_.item(serialSystem);
                            system->update(ecsDataContainer);
                        }
                });

                // once parallel systems where updates, flush their command buffers
                for (const auto& parallelSystem : parallelSystems) {
                    for (const auto& serialSystem : parallelSystem) {
                        auto* system = systemRegistry_.item(serialSystem);
                        system->flush(ecsDataContainer);
                    }
                }
            }
        }


        [[nodiscard]] bool shouldRun(EcsDataContainer& ecsDataContainer) const noexcept {
            return runCondition_(ecsDataContainer);
        }



    };

}