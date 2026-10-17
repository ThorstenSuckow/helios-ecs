module;

#include <chrono>
#include <cstddef>
#include <type_traits>
#include "helios-ecs-config.h"

export module helios.ecs.scheduling.types:RuntimeSystemEntry;

import helios.ecs.system.types;
import helios.ecs.manager.types;

export namespace helios::ecs::scheduling::types {

    template<typename TRuntimeSystemTypeId>
    struct RuntimeSystemEntryHandle {

        using TypeId = std::remove_cvref_t<TRuntimeSystemTypeId>;

        TRuntimeSystemTypeId runtimeSystemTypeId;

        std::size_t scheduleIdx;
        std::size_t parallelIdx;
        std::size_t sequentialIdx;
        std::size_t runtimeIdx;

    };


    template<typename TRuntimeSystemTypeId>
    struct RuntimeSystemEntry {

        TRuntimeSystemTypeId runtimeSystemTypeId;

#if (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_RUNTIME_SYSTEMS)
        /**
         * @brief represents main method duration, i.e. systems.update()
         * and manager.execute(), respectively
         */
        std::chrono::nanoseconds::rep updateDuration;

        /**
         * @brief flushing the command buffer is treated separately
         */
        std::chrono::nanoseconds::rep flushDuration;
#endif

    };


};