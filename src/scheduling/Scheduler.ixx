/**
 * @file Phase.ixx
 * @brief Represents a phase within the game loop containing multiple passes.
 */
module;

#include <memory>
#include <vector>

export module helios.ecs.scheduling.Scheduler;

import :BaseSchedule;
import :Schedule;

import helios.core.thread.JobSystem;

export namespace helios::ecs::scheduling {



    /**
     * @brief Represents a phase in the game loop containing multiple passes.
     */
    class Scheduler {


        using EcsDataContainer =  ecs::common::container::EcsDataContainer;
        using JobSystem = helios::core::thread::JobSystem;

        /**
         * @brief Collection of passes belonging to this phase.
         */
        std::vector<std::unique_ptr<BaseSchedule>> schedules_;

        EcsDataContainer ecsDataContainer_{};


    public:

        /**
         * @brief Constructs a Phase with references to GameLoop and GameWorld.
         *
         */
        void init(EcsDataContainer& parentEcsDataContainer) {
            ecsDataContainer_.borrow(parentEcsDataContainer);
        }

        /**
       * @brief Updates all passes within this phase.
       *
       * @param updateContext The current update context.
       * @param ecsDataContainer The map of results from the current frame's system executions.
       * @param jobSystem The job system used for parallel execution of systems.
       */
        void update(ecs::common::container::EcsDataContainer& ecsDataContainer, JobSystem& jobSystem){

            for (auto& schedule : schedules_) {

                if (schedule->shouldRun(ecsDataContainer)) {
                    schedule->update(ecsDataContainer, jobSystem);
                    schedule->onScheduleEnd(ecsDataContainer);
                }
            }
        };

        /**
         * @brief Creates and adds a new typed schedule to this phase.
         *
         * @tparam StateType The state enum type (e.g., GameState, MatchState).
         *
         * @param t The state mask specifying when this schedule should run.
         *
         * @return Reference to the newly created Pass for method chaining.
         *
         * @see TypedPass
         */
        template<typename TFunc>
        auto& beginSchedule(TFunc&& func) {
            using Predicate = std::remove_cvref_t<TFunc>;
            auto entry = std::make_unique<Schedule<Predicate>>(
                *this,
                ecsDataContainer_,
                std::forward<TFunc>(func)
            );
            auto* raw = entry.get();
            schedules_.emplace_back(std::move(entry));
            return *raw;
        }

        auto& beginSchedule() {
            auto l = []()->bool{return true;};
            auto entry = std::make_unique<Schedule<decltype(l)>>(
                *this,
                ecsDataContainer_, l

            );
            auto* raw = entry.get();
            schedules_.emplace_back(std::move(entry));
            return *raw;
        }


    };

}