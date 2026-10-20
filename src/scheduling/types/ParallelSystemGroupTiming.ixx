module;

#include <chrono>
#include <cstddef>
#include "helios-ecs-config.h"

export module helios.ecs.scheduling.types:ParallelSystemGroupTiming;

import :RuntimeSystemEntry;

export namespace helios::ecs::scheduling::types {

    /**
     * @brief Identifies a group of a schedule: `groupIdx` is the position of the group in the queue of `phase`
     * (Start = start managers, System = systems, End = end managers) of the schedule `scheduleIdx`. As for
     * RuntimeSystemEntryHandle, the indices start at 0 in every phase. Every group of the queue is measured; a group
     * with a single parallel entry is a parallel group with one entry.
     */
    struct ParallelSystemGroupEntryHandle {
        SchedulePhase phase;
        std::size_t scheduleIdx;
        std::size_t groupIdx;
    };

    /**
     * @brief Duration of a parallel system group. Members are only available if HELIOS_PERFORMANCE
     * contains HELIOS_PERFORMANCE_PARALLEL_SYSTEMGROUPS, otherwise the struct is empty.
     *
     * The duration covers the complete group: parallel execution of all runtime systems (including the thread pool
     * hand-off) and the subsequent flush of their command buffers. The (cheap) invalidation of the result views
     * is not included.
     */
    struct ParallelSystemGroupTiming {
#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_PARALLEL_SYSTEMGROUPS)
        std::chrono::nanoseconds::rep duration = 0;

        ParallelSystemGroupTiming operator+(const ParallelSystemGroupTiming& other) const {
            return {duration + other.duration};
        }

        [[nodiscard]] std::chrono::nanoseconds::rep total() const {
            return duration;
        }
#endif
    };

}
