module;

#include <chrono>
#include "helios-ecs-config.h"

export module helios.ecs.scheduling.types:ScheduleTiming;

import helios.ecs.system.types;
import helios.ecs.manager.types;

export namespace helios::ecs::scheduling::types {

    /**
     * @brief Schedule durations. Members are only available if HELIOS_PERFORMANCE
     * contains HELIOS_PERFORMANCE_SCHEDULES, otherwise the struct is empty.
     */
    struct ScheduleTiming {
#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_SCHEDULES)

        std::chrono::nanoseconds::rep scheduleStartDuration;
        std::chrono::nanoseconds::rep scheduleUpdateDuration;
        std::chrono::nanoseconds::rep scheduleEndDuration;

        ScheduleTiming operator +(const ScheduleTiming& other) const {
            return {
                scheduleStartDuration + other.scheduleStartDuration,
                scheduleUpdateDuration + other.scheduleUpdateDuration,
                scheduleEndDuration + other.scheduleEndDuration
            };
        }

        [[nodiscard]] std::chrono::nanoseconds::rep total() const {
            return scheduleStartDuration + scheduleUpdateDuration + scheduleEndDuration;
        }
#endif
    };


};