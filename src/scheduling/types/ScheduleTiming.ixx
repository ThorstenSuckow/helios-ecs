module;

#include <chrono>

export module helios.ecs.scheduling.types:ScheduleTiming;

import helios.ecs.system.types;
import helios.ecs.manager.types;

export namespace helios::ecs::scheduling::types {

    struct ScheduleTiming {
        std::chrono::nanoseconds::rep scheduleUpdateDuration;
        std::chrono::nanoseconds::rep scheduleEndDuration;

        ScheduleTiming operator +(const ScheduleTiming& other) const {
            return {
                scheduleUpdateDuration + other.scheduleUpdateDuration,
                scheduleEndDuration + other.scheduleEndDuration
            };
        }

        [[nodiscard]] std::chrono::nanoseconds::rep total() const {
            return scheduleUpdateDuration + scheduleEndDuration;
        }


    };


};