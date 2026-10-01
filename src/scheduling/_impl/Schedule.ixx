/**
 * @file TypedPass.ixx
 * @brief State-filtered pass implementation for game loop phases.
 */
module;

#include <type_traits>
#include <utility>
#include <cassert>
#include <functional>
#include "helios-ecs-config.h"


export module helios.ecs.scheduling.Scheduler:Schedule;

import :BaseSchedule;

import helios.core.thread.JobSystem;


using namespace helios::core::thread;
export namespace helios::ecs::scheduling {

    class Phase;

    /**
     * @brief State-filtered pass that only executes in specific states and if arbitrary conditions are satisifed.
     *
     * @tparam StateType The state enum type (e.g., GameState, MatchState).
     *
     * @see Pass
     * @see Phase::beginPass()
     * @see Session::state()
     */
    template<typename TPredicate>
    class Schedule : public BaseSchedule {

        friend class helios::ecs::scheduling::Scheduler;




        /**
         * @brief List of run conditions that must be satisfied for this pass to be executed.
         */
        TPredicate runCondition_;

        using BaseSchedule::owner_;
        using BaseSchedule::systemRegistry_;
        using BaseSchedule::systemTypeIdQueue_;
        using BaseSchedule::ecsDataContainer_;
        using BaseSchedule::managerTypeIds;
        using EcsDataContainer = ecs::common::container::EcsDataContainer;
        using JobSystem = helios::core::thread::JobSystem;

    protected:
        /**
         * @copydoc Pass::update
         */
        void update(EcsDataContainer& ecsDataContainer, JobSystem& jobSystem) override {


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

                // parallelSystems > 1 will be queued with the JobSystems
                jobSystem.runAndWait(
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

        /**
         * @copydoc Pass::onPassEnd
         */
        void onScheduleEnd(EcsDataContainer& ecsDataContainer) noexcept override {

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

       /**
        * @copydoc Pass::shouldRun
        */
        [[nodiscard]] bool shouldRun(EcsDataContainer& ecsDataContainer) const noexcept override {
            return common::container::EcsDataContainerFunctionInvoker::invoke<&TPredicate::operator()>(
                runCondition_, ecsDataContainer
            );
        }

        public:

        /**
         * @brief Constructs a typed pass for a state mask.
         *
         * @param owner Reference to the parent Phase.
         * @param mask State mask controlling when this pass runs.
         */
        explicit Schedule(Scheduler& owner, EcsDataContainer& parentEcsDataContainer, TPredicate runCondition)
        : BaseSchedule(owner), runCondition_(std::move(runCondition)) {
            ecsDataContainer_.borrow(parentEcsDataContainer);
        }




    };

}