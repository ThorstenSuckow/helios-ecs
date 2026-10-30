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

#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_SCHEDULES)
        types::ScheduleTiming scheduleTiming_{};
#endif

#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_PARALLEL_SYSTEMGROUPS)
        std::vector<types::ParallelSystemGroupTiming> startGroupTimings_{};
        std::vector<types::ParallelSystemGroupTiming> systemGroupTimings_{};
        std::vector<types::ParallelSystemGroupTiming> endGroupTimings_{};

        std::vector<types::ParallelSystemGroupTiming>& groupTimingsFor(types::SchedulePhase phase) {
            switch (phase) {
                case types::SchedulePhase::Start: return startGroupTimings_;
                case types::SchedulePhase::End:   return endGroupTimings_;
                default:                          return systemGroupTimings_;
            }
        }

        const std::vector<types::ParallelSystemGroupTiming>& groupTimingsFor(types::SchedulePhase phase) const {
            switch (phase) {
                case types::SchedulePhase::Start: return startGroupTimings_;
                case types::SchedulePhase::End:   return endGroupTimings_;
                default:                          return systemGroupTimings_;
            }
        }

        void addGroupTiming(types::SchedulePhase phase) {
            groupTimingsFor(phase).emplace_back();
        }
#else
        void addGroupTiming(types::SchedulePhase) {}
#endif

        std::vector<std::vector<std::vector<SystemExecutionEntry>>> systemTypeIdQueue_;

        std::vector<bool> registeredManagerTypeIds_;

        using ManagerQueue = std::vector<std::vector<std::vector<ManagerExecutionEntry>>>;
        ManagerQueue managerTypeIdEndQueue_;
        ManagerQueue managerTypeIdStartQueue_;

        bool isScheduleStartConfigurationPossible_ = true;

        bool checkScheduleStartConfigurationPossible() {
            if (!isScheduleStartConfigurationPossible_) {
                assert(false && "Schedule start configuration is not possible anymore.");
                std::terminate();
            }
            isScheduleStartConfigurationPossible_ = false;
            return true;
        }

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
                        (void)em.template ensureSparseSet<TComponent>();
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
        runCondition_([predicate](auto& ecsDataContainer) mutable -> bool {
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
            isScheduleStartConfigurationPossible_ = false;
            using SystemType = std::remove_cvref_t<TSystem>;
            return add(SystemType{});
        }

        template<typename TSystem>
        requires ecs::system::concepts::IsTypedSystem<TSystem>
        Schedule& add(TSystem sys) {
            isScheduleStartConfigurationPossible_ = false;
            using SystemType = std::remove_cvref_t<TSystem>;
            ensureRequiredStorage<SystemType>(ecsDataContainer_);
            registerTypedSystemInstance<SystemType>(std::move(sys));
            systemTypeIdQueue_.push_back({{SystemExecutionEntry{ecs::system::types::SystemTypeId::template id<SystemType>()}}});
            addGroupTiming(types::SchedulePhase::System);
            return *this;
        }

        template<typename TFuncSystem>
        requires ecs::system::concepts::IsCallableSystem<TFuncSystem>
        Schedule& add(TFuncSystem&& system) {
            isScheduleStartConfigurationPossible_ = false;
            using SystemType = std::remove_cvref_t<TFuncSystem>;

            ensureRequiredStorage<SystemType>(ecsDataContainer_);

            registerCallOperatorSystem(std::forward<TFuncSystem>(system));
            systemTypeIdQueue_.push_back({{SystemExecutionEntry{ecs::system::types::SystemTypeId::template id<SystemType>()}}});
            addGroupTiming(types::SchedulePhase::System);
            return *this;
        }
        // +---------------------------------
        // +---------------------------------


        template<typename ... TSystem>
        requires concepts::ConflictFreeCallableSystems<TSystem...>
        Schedule& add(TSystem&&... system) {
            isScheduleStartConfigurationPossible_ = false;
            (ensureRequiredStorage<TSystem>(ecsDataContainer_), ...);

            (registerCallOperatorSystem(std::forward<TSystem>(system)), ...);

            auto& group = systemTypeIdQueue_.emplace_back();
            addGroupTiming(types::SchedulePhase::System);

            group.reserve(sizeof...(TSystem));
            (group.push_back({SystemExecutionEntry{ecs::system::types::SystemTypeId::template id<std::remove_cvref_t<TSystem>>()}}), ...);

            return *this;
        }


        template<typename ...TSystem>
        requires concepts::ConflictFreeTypedSystems<TSystem...>
        Schedule& add() {
            isScheduleStartConfigurationPossible_ = false;
            (ensureRequiredStorage<TSystem>(ecsDataContainer_), ...);

            (registerTypedSystemInstance<TSystem>(std::remove_cvref_t<TSystem>{}), ...);

            auto& group = systemTypeIdQueue_.emplace_back();
            addGroupTiming(types::SchedulePhase::System);
            group.reserve(sizeof...(TSystem));
            (group.push_back({SystemExecutionEntry{ecs::system::types::SystemTypeId::template id<TSystem>()}}), ...);

            return *this;
        }

        template<typename ...TSystem>
        requires concepts::ConflictFreeTypedSystems<TSystem...>
        Schedule& add(TSystem&& ...systems) {
            isScheduleStartConfigurationPossible_ = false;
            (ensureRequiredStorage<TSystem>(ecsDataContainer_), ...);

            (registerTypedSystemInstance(std::move(systems)), ...);

            auto& group = systemTypeIdQueue_.emplace_back();
            addGroupTiming(types::SchedulePhase::System);
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
            isScheduleStartConfigurationPossible_ = false;
            auto& parallelGroup = systemTypeIdQueue_.emplace_back();
            addGroupTiming(types::SchedulePhase::System);
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

        template<typename... TManagers>
        requires concepts::ConflictFreeManagers<TManagers...>
        Schedule& startWith() {
            checkScheduleStartConfigurationPossible();
            enqueueManager<common::Sequential<TManagers>...>(managerTypeIdStartQueue_);
            return *this;
        }

        template<typename... TManagers>
        requires concepts::ConflictFreeManagers<TManagers...>
        Schedule& startWith(TManagers&& ... managers) {
            checkScheduleStartConfigurationPossible();
            enqueueManager(
                managerTypeIdStartQueue_,
                common::Sequential(std::forward<TManagers>(managers))...
            );
            return *this;
        }

        template <typename ... TSequentials>
        requires concepts::ConflictFreeSequentialManagers<TSequentials...>
        Schedule& startWith(TSequentials&&... sequentials) {
            checkScheduleStartConfigurationPossible();
            enqueueManager(managerTypeIdStartQueue_, std::forward<TSequentials>(sequentials)...);
            return *this;
        }

        template <typename ... TSequentials>
        requires concepts::ConflictFreeSequentialManagers<TSequentials...>
        Scheduler& startWith() {
            enqueueManager<TSequentials...>(managerTypeIdStartQueue_);
            return owner_;
        }


        Scheduler& endSchedule() {
            return owner_;
        }

        template<typename... TManagers>
        requires concepts::ConflictFreeManagers<TManagers...>
        Scheduler& endWith() {
            enqueueManager<common::Sequential<TManagers>...>(managerTypeIdEndQueue_);
            return owner_;
        }

        template<typename... TManagers>
        requires concepts::ConflictFreeManagers<TManagers...>
        Scheduler& endWith(TManagers&& ... managers) {
            enqueueManager(
                managerTypeIdEndQueue_,
                common::Sequential(std::forward<TManagers>(managers))...
            );
            return owner_;
        }

        template <typename ... TSequentials>
       requires concepts::ConflictFreeSequentialManagers<TSequentials...>
       Scheduler& endWith(TSequentials&&... sequentials) {
            enqueueManager(managerTypeIdEndQueue_, std::forward<TSequentials>(sequentials)...);
            return owner_;
        }

        template <typename ... TSequentials>
        requires concepts::ConflictFreeSequentialManagers<TSequentials...>
        Scheduler& endWith() {
            enqueueManager<TSequentials...>(managerTypeIdEndQueue_);
            return owner_;
        }

        template <typename ... TSequentials>
        requires concepts::ConflictFreeSequentialManagers<TSequentials...>
        Scheduler& enqueueManager(ManagerQueue& managerQueue, TSequentials&&... sequentials) {
            isScheduleStartConfigurationPossible_ = false;
            auto& parallelGroup = managerQueue.emplace_back();
            addGroupTiming(&managerQueue == &managerTypeIdEndQueue_
                ? types::SchedulePhase::End : types::SchedulePhase::Start);
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
        Scheduler& enqueueManager(ManagerQueue& managerQueue) {
            isScheduleStartConfigurationPossible_ = false;
            auto& parallelGroup = managerQueue.emplace_back();
            addGroupTiming(&managerQueue == &managerTypeIdEndQueue_
                ? types::SchedulePhase::End : types::SchedulePhase::Start);
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

        void onScheduleSkip() {
#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_RUNTIME_SYSTEMS)
            auto resetTimings = [](auto&  queue) {
                for (std::size_t groupIdx = 0; groupIdx < queue.size(); ++groupIdx) {
                    auto& parallelRuntimeSystems = queue[groupIdx];
                    for (auto& sequentials : parallelRuntimeSystems) {
                        for (auto& serialRuntimeSystem : sequentials) {
                            serialRuntimeSystem.reset();
                        }
                    }
                }
            };
            resetTimings(systemTypeIdQueue_);
            resetTimings(managerTypeIdStartQueue_);
            resetTimings(managerTypeIdEndQueue_);
#endif
        }


        void onScheduleStart() noexcept {
            runManagers(managerTypeIdStartQueue_, types::SchedulePhase::Start);
        }

        void onScheduleEnd() noexcept {
            runManagers(managerTypeIdEndQueue_, types::SchedulePhase::End);
        }

        // +---------------------------
        // | Runtime
        // +---------------------------
        void runManagers(ManagerQueue& managerQueue, types::SchedulePhase phase) noexcept {

            if (managerQueue.empty()) {
                return;
            }

            auto* reg = ecsDataContainer_.tryGet<ecs::manager::ManagerRegistry>();
            #if HELIOS_DEBUG
            if (!reg) {
                assert(reg && "ManagerRegistry not found in EcsDataContainer");
            }
            #endif

#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_SCHEDULES)
            const auto stopwatch = helios::core::time::Stopwatch{};
#endif
            run(managerQueue, *reg, [](auto& manager, auto& ecsDataContainer) {
                manager.execute(ecsDataContainer);
            }, phase);
#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_SCHEDULES)
            auto elapsed = stopwatch.elapsedNanoseconds();
            if (phase == types::SchedulePhase::End) {
                scheduleTiming_.scheduleEndDuration = elapsed;
            } else if (phase == types::SchedulePhase::Start) {
                scheduleTiming_.scheduleStartDuration = elapsed;
            } else {
                assert(false && "Unexpected phase for manager execution");
                std::terminate();
            }
#endif
        }

        void update() {
#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_SCHEDULES)
            const auto stopwatch = helios::core::time::Stopwatch{};
#endif
            run(systemTypeIdQueue_, systemRegistry_, [](auto& system, auto& ecsDataContainer) {
                system.update(ecsDataContainer);
            }, types::SchedulePhase::System);
#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_SCHEDULES)
            scheduleTiming_.scheduleUpdateDuration = stopwatch.elapsedNanoseconds();
#endif
        }

        template<typename TRuntimeQueue, typename TRegistry, typename TExecute>
        void run(TRuntimeQueue& queue, TRegistry& registry, TExecute cmd, [[maybe_unused]] const types::SchedulePhase phase) {

            auto runtimeSystemFor = [&registry](auto &entry) {
                return registry.item(entry.runtimeSystemTypeId);
            };

            auto invalidateView = [this](bool mustInvalidate) {
                if (mustInvalidate) {
                    if (auto* resultRegistry = ecsDataContainer_.tryGet<ecs::common::RuntimeResultRegistry>()) {
                        resultRegistry->invalidateView();
                    }
                }
            };

            auto* threadPool = ecsDataContainer_.tryGet<ThreadPool>();
            assert(threadPool && "ThreadPool not found in EcsDataContainer");

#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_PARALLEL_SYSTEMGROUPS)
            auto& groupTimings = groupTimingsFor(phase);
            assert(groupTimings.size() == queue.size() && "Group timings must be parallel to the queue");
#endif

            for (std::size_t groupIdx = 0; groupIdx < queue.size(); ++groupIdx) {
                auto& parallelRuntimeSystems = queue[groupIdx];
                bool mustInvalidate = false;

#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_PARALLEL_SYSTEMGROUPS)
                // one stopwatch per group (a group with a single entry is a parallel group with one parallel entry):
                // execution and flushes (the cheap view invalidation is not part of the duration)
                [[maybe_unused]] const auto groupStopwatch = helios::core::time::Stopwatch{};
#endif

                // a single parallel entry does not need the ThreadPool and is executed on the calling thread
                if (parallelRuntimeSystems.size() == 1) {
                    for (auto& serialRuntimeSystem : parallelRuntimeSystems[0]) {

                        auto* runtimeSystem = runtimeSystemFor(serialRuntimeSystem);

                        // update, commit mutations
#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_RUNTIME_SYSTEMS)
                        auto stopwatch = helios::core::time::Stopwatch{};
                        cmd(*runtimeSystem, ecsDataContainer_);
                        serialRuntimeSystem.updateDuration = stopwatch.elapsedNanoseconds();
                        serialRuntimeSystem.threadId = helios::core::thread::ThreadPool::currentThreadId();

                        // produce system results, flush any underlying flushable objects
                        stopwatch.start();
                        runtimeSystem->flush(ecsDataContainer_);
                        serialRuntimeSystem.flushDuration = stopwatch.elapsedNanoseconds();
#else
                        cmd(*runtimeSystem, ecsDataContainer_);
                        runtimeSystem->flush(ecsDataContainer_);
#endif

                        if (runtimeSystem->hasResult()) {
                            mustInvalidate = true;
                        }

                    }
#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_PARALLEL_SYSTEMGROUPS)
                    groupTimings[groupIdx].duration = groupStopwatch.elapsedNanoseconds();
#endif
                    invalidateView(mustInvalidate);

                    continue;
                }

                // parallelSystems > 1 will be queued with the ThreadPool
                threadPool->runAndWait(
                    parallelRuntimeSystems.size(),
                    [&] (const std::size_t i) {
                        // a parallel system owns more ore more serial systems
                        for (auto& serialRuntimeSystem : parallelRuntimeSystems[i]) {
                            auto* runtimeSystem = runtimeSystemFor(serialRuntimeSystem);

#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_RUNTIME_SYSTEMS)
                            auto stopwatch = helios::core::time::Stopwatch{};
                            cmd(*runtimeSystem, ecsDataContainer_);
                            serialRuntimeSystem.updateDuration = stopwatch.elapsedNanoseconds();
                            serialRuntimeSystem.threadId = helios::core::thread::ThreadPool::currentThreadId();
#else
                            cmd(*runtimeSystem, ecsDataContainer_);
#endif

                        }
                });

                // once parallel systems where updates, flush their command buffers
                for (auto& parallelRuntimeSystem : parallelRuntimeSystems) {
                    for (auto& serialRuntimeSystem : parallelRuntimeSystem) {
                        auto* runtimeSystem = runtimeSystemFor(serialRuntimeSystem);

#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_RUNTIME_SYSTEMS)
                        auto stopwatch = helios::core::time::Stopwatch{};
                        runtimeSystem->flush(ecsDataContainer_);
                        serialRuntimeSystem.flushDuration = stopwatch.elapsedNanoseconds();
#else
                        runtimeSystem->flush(ecsDataContainer_);
#endif

                        if (runtimeSystem->hasResult()) {
                            mustInvalidate = true;
                        }
                    }
                }
#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_PARALLEL_SYSTEMGROUPS)
                groupTimings[groupIdx].duration = groupStopwatch.elapsedNanoseconds();
#endif
                invalidateView(mustInvalidate);
            }

        }

        [[nodiscard]] bool shouldRun() noexcept {
            return runCondition_(ecsDataContainer_);
        }

        std::size_t idx() {
            return idx_;
        }

#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_SCHEDULES)
        types::ScheduleTiming timings() const {
            return scheduleTiming_;
        }
#endif


#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_PARALLEL_SYSTEMGROUPS)
        types::ParallelSystemGroupTiming timings(types::ParallelSystemGroupEntryHandle handle) const {
            assert(handle.scheduleIdx == idx_ && "Unexpected idx mismatch.");

            const auto& groupTimings = groupTimingsFor(handle.phase);
            assert(handle.groupIdx < groupTimings.size() && "Unexpected groupIdx");

            return groupTimings[handle.groupIdx];
        }
#endif

#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_RUNTIME_SYSTEMS)
        template<typename TRuntimeSystemTypeId>
        const types::RuntimeSystemEntry<TRuntimeSystemTypeId> timings(types::RuntimeSystemEntryHandle<TRuntimeSystemTypeId> handle) const {
            assert(handle.scheduleIdx == idx_ && "Unexpected idx mismatch.");

            auto queueFor = [&](types::RuntimeSystemEntryHandle<TRuntimeSystemTypeId> hndl) -> const auto& {
                using TypeId = std::remove_cvref_t<TRuntimeSystemTypeId>;

                if constexpr (std::same_as<TypeId,  manager::types::ManagerTypeId>) {
                    if (hndl.phase == types::SchedulePhase::Start) {
                        return managerTypeIdStartQueue_;
                    }
                    if (hndl.phase == types::SchedulePhase::End) {
                        return managerTypeIdEndQueue_;
                    }
                    assert(false && "Unexpected phase for manager");
                    std::terminate();
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
#endif

    };

}