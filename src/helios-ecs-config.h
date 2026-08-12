#pragma once
#ifndef NDEBUG
#define HELIOS_DEBUG 1
#endif
#define DEFAULT_ENTITY_MANAGER_CAPACITY 1000
#define DEFAULT_ENTITY_MUTATION_COMMAND_BUFFER_CAPACITY  1000

#if defined(_MSC_VER)
    #define HELIOS_FUNCTION_SIGNATURE __FUNCSIG__
#elif defined(__clang__) || defined(__GNUC__)
    #define HELIOS_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#else
    #define HELIOS_FUNCTION_SIGNATURE __func__
#endif

