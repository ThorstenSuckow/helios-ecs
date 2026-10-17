#pragma once
#ifndef NDEBUG
#define HELIOS_DEBUG 1
#endif

/**
 * Compile-time performance measurement of the scheduler (bit mask).
 *
 *  HELIOS_PERFORMANCE_SCHEDULES       (1): duration of each schedule (update + schedule end)
 *  HELIOS_PERFORMANCE_RUNTIME_SYSTEMS (2): duration of each system update / manager execution
 *                                          and of each command buffer flush
 *  HELIOS_PERFORMANCE_ALL             (3): both
 *
 * Default is 0: no measurement code is compiled, ScheduleTiming and RuntimeSystemEntry
 * carry no duration members. Set it as a plain compiler definition, e.g. -DHELIOS_PERFORMANCE=3
 * (like HEADLESS). Sources test the named bits, e.g. (HELIOS_PERFORMANCE & HELIOS_PERFORMANCE_SCHEDULES),
 * and therefore must include this header.
 */
#define HELIOS_PERFORMANCE_NONE 0
#define HELIOS_PERFORMANCE_SCHEDULES 1
#define HELIOS_PERFORMANCE_RUNTIME_SYSTEMS 2
#define HELIOS_PERFORMANCE_ALL 3

#ifndef HELIOS_PERFORMANCE
#define HELIOS_PERFORMANCE HELIOS_PERFORMANCE_NONE
#endif

#define DEFAULT_ENTITY_MANAGER_CAPACITY 1000
#define DEFAULT_ENTITY_MUTATION_COMMAND_BUFFER_CAPACITY 1000

#if defined(_MSC_VER)
#define HELIOS_FUNCTION_SIGNATURE __FUNCSIG__
#elif defined(__clang__) || defined(__GNUC__)
#define HELIOS_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#else
#define HELIOS_FUNCTION_SIGNATURE __func__
#endif
