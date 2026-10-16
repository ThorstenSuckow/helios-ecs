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
import helios.core.time;

import helios.ecs.scheduling.traits;
import helios.ecs.scheduling.concepts;
import helios.ecs.scheduling.types;


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

        using SystemExecutionEntry = types::RuntimeSystemEntry<system::types::SystemTypeId>;
        using ManagerExecutionEntry = types::RuntimeSystemEntry<manager::types::ManagerTypeId>;

        ecs::system::SystemRegistry systemRegistry_{};

        types::ScheduleTiming scheduleTiming_{};

        std::vector<std::vector<std::vector<SystemExecutionEntry>>> systemTypeIdQueue_;

        std::vector<bool> registeredManagerTypeIds_;
        std::vector<std::vector<std::vector<ManagerExecutionEntry>>> managerTypeIdQueue_;

        template<typename TManager>
        void prepareManagerRegistration() {
            using ManagerType = std::remove_cvref_t<TManager>;
            auto typeId = manager::types::ManagerTypeId::template id<ManagerType>();
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
        }

        template<typename TManager>
        void registerManagerInstance(TManager&& mgr) {
            prepareManagerRegistration<TManager>();
            using ManagerType = std::remove_cvref_t<TManager>;
            auto typeId = manager::types::ManagerTypeId::template id<ManagerType>();
            auto* reg = ecsDataContainer_.tryGet<ecs::manager::ManagerRegistry>();

            auto* manager = reg->item(typeId);
            if (!manager) {
                reg->add<ManagerType>(std::forward<TManager>(mgr));
            } else {
                assert(false && "Manager already registered, provide Type only in this case.");
                std::terminate();
            }

        }

        template<typename TManager>
        void registerManager() {
            prepareManagerRegistration<TManager>();
            using ManagerType = std::remove_cvref_t<TManager>;
            auto typeId = manager::types::ManagerTypeId::template id<ManagerType>();
            auto* reg = ecsDataContainer_.tryGet<ecs::manager::ManagerRegistry>();


            auto* manager = reg->item(typeId);
            if (!manager) {
                 if constexpr (std::is_default_constructible_v<ManagerType>) {
                    reg->add<ManagerType>(ManagerType{});
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

        std::size_t idx_;

    public:

        Schedule(const Schedule&) = delete;
        Schedule& operator=(const Schedule&) = delete;
        Schedule(Schedule&&) noexcept = delete;
        Schedule& operator=(Schedule&&) noexcept = delete;



        template<typename TPredicate>
        explicit Schedule(
            Scheduler& owner,
            EcsDataContainer& parentEcsDataContainer,
            TPredicate predicate,
            std::size_t idx
        )
        : owner_(owner),
        runCondition_([predicate](auto& ecsDataContainer) -> bool {
            return common::container::EcsDataContainerFunctionInvoker::invoke<&TPredicate::operator()>(
            predicate, ecsDataContainer);
        }),
        idx_(idx)
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
            systemTypeIdQueue_.push_back({{SystemExecutionEntry{ecs::system::types::SystemTypeId::template id<SystemType>()}}});
            return *this;
        }

        template<typename TFuncSystem>
        requires ecs::system::concepts::IsCallableSystem<TFuncSystem>
        Schedule& add(TFuncSystem&& system) {
            using SystemType = std::remove_cvref_t<TFuncSystem>;

            ensureRequiredStorage<SystemType>(ecsDataContainer_);

            registerCallOperatorSystem(std::forward<TFuncSystem>(system));
            systemTypeIdQueue_.push_back({{SystemExecutionEntry{ecs::system::types::SystemTypeId::template id<SystemType>()}}});
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
            (group.push_back({SystemExecutionEntry{ecs::system::types::SystemTypeId::template id<std::remove_cvref_t<TSystem>>()}}), ...);

            return *this;
        }


        template<typename ...TSystem>
        requires concepts::ConflictFreeTypedSystems<TSystem...>
        Schedule& add() {

            (ensureRequiredStorage<TSystem>(ecsDataContainer_), ...);

            (registerTypedSystemInstance<TSystem>(std::remove_cvref_t<TSystem>{}), ...);

            auto& group = systemTypeIdQueue_.emplace_back();
            group.reserve(sizeof...(TSystem));
            (group.push_back({SystemExecutionEntry{ecs::system::types::SystemTypeId::template id<TSystem>()}}), ...);

            return *this;
        }

        template<typename ...TSystem>
        requires concepts::ConflictFreeTypedSystems<TSystem...>
        Schedule& add(TSystem&& ...systems) {

            (ensureRequiredStorage<TSystem>(ecsDataContainer_), ...);

            (registerTypedSystemInstance(std::move(systems)), ...);

            auto& group = systemTypeIdQueue_.emplace_back();
            group.reserve(sizeof...(TSystem));
            (group.push_back({SystemExecutionEntry{ecs::system::types::SystemTypeId::template id<std::remove_cvref_t<TSystem>>()}}), ...);

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

                auto& sys = sequential.template runtimeSystemFor<TSystem>();

                if constexpr(ecs::system::concepts::IsCallableSystem<TSystem>) {
                    registerCallOperatorSystem(std::forward<TSystem>(sys));
                } else {
                    registerTypedSystemInstance<TSystem>(std::move(sys));
                }
                sequentialGroup.push_back(SystemExecutionEntry{ecs::system::types::SystemTypeId::template id<TSystem>()});
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
        Scheduler& endSchedule(TSequentials&&... sequentials) {
            auto& parallelGroup = managerTypeIdQueue_.emplace_back();
            parallelGroup.reserve(sizeof...(TSequentials));

            auto addManagerInstance = [this]<typename TManager>(auto& sequential, auto& sequentialGroup) {

                ensureRequiredStorage<TManager>(ecsDataContainer_);

                auto& mgr = sequential.template runtimeSystemFor<TManager>();

                registerManagerInstance<TManager>(std::move(mgr));
                sequentialGroup.push_back(ManagerExecutionEntry{ecs::manager::types::ManagerTypeId::template id<TManager>()});
            };

            auto registerManagers = [this, &addManagerInstance]<typename ...TManagers>
            (common::Sequential<TManagers...>& sequential, auto& sequentialGroup) {
                sequentialGroup.reserve(sizeof...(TManagers));
                (addManagerInstance.template operator()<TManagers>(sequential, sequentialGroup), ...);
            };


            (registerManagers(sequentials, parallelGroup.emplace_back()), ...);
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
                sequentialGroup.push_back(ManagerExecutionEntry{ecs::manager::types::ManagerTypeId::template id<TManager>()});
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
        void onScheduleEnd(const bool collectScheduleTimings = false, const bool collectRuntimeSystemEntrys = false) noexcept {

            if (managerTypeIdQueue_.empty()) {
                return;
            }

            auto* reg = ecsDataContainer_.tryGet<ecs::manager::ManagerRegistry>();
            #if HELIOS_DEBUG
            if (!reg) {
                assert(reg && "ManagerRegistry not found in EcsDataContainer");
            }
            #endif

            if (collectScheduleTimings) {
                const auto stopwatch = helios::core::time::Stopwatch{};
                run(managerTypeIdQueue_, *reg, [](auto& manager, auto& ecsDataContainer) {
                   manager.execute(ecsDataContainer);
                }, collectRuntimeSystemEntrys);
                scheduleTiming_.scheduleEndDuration = stopwatch.elapsedNanoseconds();
            } else {
                run(managerTypeIdQueue_, *reg, [](auto& manager, auto& ecsDataContainer) {
                   manager.execute(ecsDataContainer);
               }, collectRuntimeSystemEntrys);
            }
        }

        void update(const bool collectScheduleTimings = false, const bool collectRuntimeSystemEntrys = false) {

            if (collectScheduleTimings) {
                const auto stopwatch = helios::core::time::Stopwatch{};
                run(systemTypeIdQueue_, systemRegistry_, [](auto& system, auto& ecsDataContainer) {
                    system.update(ecsDataContainer);
                }, collectRuntimeSystemEntrys);
                scheduleTiming_.scheduleUpdateDuration = stopwatch.elapsedNanoseconds();
            } else {
                run(systemTypeIdQueue_, systemRegistry_, [](auto& system, auto& ecsDataContainer) {
                    system.update(ecsDataContainer);
                }, collectRuntimeSystemEntrys);
            }
        }

        template<typename TRuntimeQueue, typename TRegistry, typename TExecute>
        void run(TRuntimeQueue& queue, TRegistry& registry, TExecute cmd, bool collectRuntimeSystemEntrys = false) {

            auto runtimeSystemFor = [&registry](auto &entry) {
                using EntryType = std::remove_cvref_t<decltype(entry)>;
                return registry.item(entry.runtimeSystemTypeId);
            };


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
                    for (auto& serialRuntimeSystem : parallelRuntimeSystems[0]) {

                        auto* runtimeSystem = runtimeSystemFor(serialRuntimeSystem);

                        // update, commit mutations
                        if (collectRuntimeSystemEntrys) {
                            auto stopwatch = helios::core::time::Stopwatch{};
                            cmd(*runtimeSystem, ecsDataContainer_);
                            // produce system results, flush any underlying flushable objects
                            serialRuntimeSystem.updateDuration = stopwatch.elapsedNanoseconds();

                            stopwatch.start();
                            runtimeSystem->flush(ecsDataContainer_);
                            serialRuntimeSystem.flushDuration = stopwatch.elapsedNanoseconds();

                        } else {
                            cmd(*runtimeSystem, ecsDataContainer_);
                            runtimeSystem->flush(ecsDataContainer_);
                            serialRuntimeSystem.flushDuration = 0.0;
                            serialRuntimeSystem.updateDuration = 0.0;
                        }

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
                        for (auto& serialRuntimeSystem : parallelRuntimeSystems[i]) {
                            auto* runtimeSystem = runtimeSystemFor(serialRuntimeSystem);

                            if (collectRuntimeSystemEntrys) {
                                auto stopwatch = helios::core::time::Stopwatch{};
                                cmd(*runtimeSystem, ecsDataContainer_);
                                serialRuntimeSystem.updateDuration = stopwatch.elapsedNanoseconds();
                            } else {
                                cmd(*runtimeSystem, ecsDataContainer_);
                                serialRuntimeSystem.updateDuration = 0.0;
                            }

                        }
                });

                // once parallel systems where updates, flush their command buffers
                for (auto& parallelRuntimeSystem : parallelRuntimeSystems) {
                    for (auto& serialRuntimeSystem : parallelRuntimeSystem) {
                        auto* runtimeSystem = runtimeSystemFor(serialRuntimeSystem);

                        if (collectRuntimeSystemEntrys) {
                            auto stopwatch = helios::core::time::Stopwatch{};
                            runtimeSystem->flush(ecsDataContainer_);
                            serialRuntimeSystem.flushDuration = stopwatch.elapsedNanoseconds();
                        } else {
                            runtimeSystem->flush(ecsDataContainer_);
                            serialRuntimeSystem.flushDuration = 0.0;
                        }

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

        std::size_t idx() {
            return idx_;
        }

        types::ScheduleTiming timings() const {
            return scheduleTiming_;
        }

        template<typename TRuntimeSystemTypeId>
        const types::RuntimeSystemEntry<TRuntimeSystemTypeId> timings(types::RuntimeSystemEntryHandle<TRuntimeSystemTypeId> handle) const {
            assert(handle.scheduleIdx == idx_ && "Unexpected idx mismatch.");

            auto queueFor = [&](types::RuntimeSystemEntryHandle<TRuntimeSystemTypeId> handle) -> const auto& {
                using TypeId = std::remove_cvref_t<TRuntimeSystemTypeId>;

                if constexpr (std::same_as<TypeId,  manager::types::ManagerTypeId>) {
                    return managerTypeIdQueue_;
                } else if constexpr (std::same_as<TypeId,  system::types::SystemTypeId>) {
                    return systemTypeIdQueue_;
                }
            };

            const auto& queue = queueFor(handle);

            auto parallelIdx   = handle.parallelIdx;
            auto sequentialIdx = handle.sequentialIdx;
            auto runtimeIdx    = handle.runtimeIdx;

            assert(parallelIdx < queue.size() && "Unexpected parallelIdx");
            assert(sequentialIdx < queue[parallelIdx].size() && "Unexpected sequentialIdx");
            assert(runtimeIdx < queue[parallelIdx][sequentialIdx].size() && "Unexpected runtimeIdx");

            auto runtimeTimings = queue[parallelIdx][sequentialIdx][runtimeIdx];

            assert(runtimeTimings.runtimeSystemTypeId == handle.runtimeSystemTypeId && "Unexpected runtimeSystemTypeId");

            return runtimeTimings;
        }


    };

}