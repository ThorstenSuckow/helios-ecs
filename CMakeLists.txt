cmake_minimum_required(VERSION 4.0)
project(helios-ecs LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

include(CTest)

set(HELIOS_CORE_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-core" CACHE PATH "Local path to helios-core repository")

# Resolve helios-core when helios-ecs is built standalone.
if(NOT TARGET helios::core)
    include(FetchContent)

    set(_HELIOS_ECS_BUILD_TESTING "${BUILD_TESTING}")
    set(_HELIOS_ECS_BUILD_BENCHMARKS "${BUILD_BENCHMARKS}")
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)

    if(EXISTS "${HELIOS_CORE_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_CORE_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-core-build")
        message(STATUS "Using local helios-core: ${HELIOS_CORE_LOCAL_PATH}")
    else()
        FetchContent_Declare(helios_core
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-core.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_core)
    endif()

    if(DEFINED _HELIOS_ECS_BUILD_TESTING)
        set(BUILD_TESTING "${_HELIOS_ECS_BUILD_TESTING}" CACHE BOOL "" FORCE)
    endif()
    if(DEFINED _HELIOS_ECS_BUILD_BENCHMARKS)
        set(BUILD_BENCHMARKS "${_HELIOS_ECS_BUILD_BENCHMARKS}" CACHE BOOL "" FORCE)
    endif()
endif()

if(CMAKE_IFC_OUTPUT_DIRECTORY)
    file(MAKE_DIRECTORY "${CMAKE_IFC_OUTPUT_DIRECTORY}")
endif()

file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

file(GLOB_RECURSE MODULE_INTERFACE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx")

list(FILTER CPP_FILES EXCLUDE REGEX "/CMakeFiles/|/CMakeTmp/|/cmake_build_|/build/")
list(FILTER MODULE_INTERFACE_FILES EXCLUDE REGEX "/CMakeFiles/|/CMakeTmp/|/cmake_build_|/build/")

add_library(helios_ecs STATIC)
add_library(helios::ecs ALIAS helios_ecs)
set_property(TARGET helios_ecs PROPERTY CXX_SCAN_FOR_MODULES ON)
target_compile_features(helios_ecs PUBLIC cxx_std_23)
target_link_libraries(helios_ecs PUBLIC helios::core)

target_sources(helios_ecs PRIVATE ${CPP_FILES})

if(MODULE_INTERFACE_FILES)
    target_sources(helios_ecs PUBLIC
            FILE_SET modules TYPE CXX_MODULES
            BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
            FILES ${MODULE_INTERFACE_FILES}
    )
endif()


target_include_directories(helios_ecs PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

set(HELIOS_ECS_BUILD_TESTS_DEFAULT OFF)
if(PROJECT_IS_TOP_LEVEL)
    set(HELIOS_ECS_BUILD_TESTS_DEFAULT ON)
endif()
option(HELIOS_ECS_BUILD_TESTS "Build helios-ecs tests" ${HELIOS_ECS_BUILD_TESTS_DEFAULT})

if(HELIOS_ECS_BUILD_TESTS AND BUILD_TESTING)
    include(FetchContent)

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0
    )
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/test/src/*.test.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/src/*.test.cpp")

    add_executable(helios-ecs-tests ${TEST_SOURCES})
    target_link_libraries(helios-ecs-tests PRIVATE helios_ecs GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(helios-ecs-tests)
endif()

option(BUILD_BENCHMARKS "Build benchmarks" ON)
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

