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



    template<typename TPredicate>
    class Schedule : public BaseSchedule {

        friend class helios::ecs::scheduling::Scheduler;

        TPredicate runCondition_;

        using BaseSchedule::ecsDataContainer_;
        using EcsDataContainer = ecs::common::container::EcsDataContainer;

    protected:

        public:


        explicit Schedule(Scheduler& owner, EcsDataContainer& parentEcsDataContainer, TPredicate runCondition)
        : BaseSchedule(owner), runCondition_(std::move(runCondition)) {
            ecsDataContainer_.borrow(parentEcsDataContainer);
        }

        [[nodiscard]] bool shouldRun(EcsDataContainer& ecsDataContainer) const noexcept override {
            return common::container::EcsDataContainerFunctionInvoker::invoke<&TPredicate::operator()>(
                runCondition_, ecsDataContainer
            );
        }



    };

}