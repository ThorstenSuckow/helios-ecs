module;

#include <chrono>

export module helios.ecs.scheduling.types:RuntimeSystemTiming;

import helios.ecs.system.types;
import helios.ecs.manager.types;

export namespace helios::ecs::scheduling::types {

    template<typename TRuntimeSystemTypeId>
    struct RuntimeSystemTimingHandle {

        using TypeId = std::remove_cvref_t<TRuntimeSystemTypeId>;

        TRuntimeSystemTypeId runtimeSystemTypeId;

        std::size_t scheduleIdx;
        std::size_t parallelIdx;
        std::size_t sequentialIdx;
        std::size_t runtimeIdx;

    };


    template<typename TRuntimeSystemTypeId>
    struct RuntimeSystemTiming {

        TRuntimeSystemTypeId runtimeSystemTypeId;

        /**
         * @brief represents main method duration, i.e. systems.update()
         * and manager.execute(), respectively
         */
        std::chrono::nanoseconds::rep updateDuration;

        /**
         * @brief flishign the command buffer is treated separately
         */
        std::chrono::nanoseconds::rep flushDuration;

    };


};