cmake_minimum_required(VERSION 4.0)
project(helios-ecs LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

include(CTest)

file(MAKE_DIRECTORY "${CMAKE_IFC_OUTPUT_DIRECTORY}")

file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

file(GLOB_RECURSE MODULE_INTERFACE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx")

list(FILTER CPP_FILES EXCLUDE REGEX "/CMakeFiles/|/CMakeTmp/|/cmake_build_|/build/")
list(FILTER MODULE_INTERFACE_FILES EXCLUDE REGEX "/CMakeFiles/|/CMakeTmp/|/cmake_build_|/build/")

add_library(helios STATIC)
set_property(TARGET helios PROPERTY CXX_SCAN_FOR_MODULES ON)
target_compile_features(helios PUBLIC cxx_std_23)

target_sources(helios PRIVATE ${CPP_FILES})

if(MODULE_INTERFACE_FILES)
    target_sources(helios PUBLIC
            FILE_SET modules TYPE CXX_MODULES
            BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
            FILES ${MODULE_INTERFACE_FILES}
    )
endif()


target_include_directories(helios PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${HELIOS_ECS_GENERATED_INCLUDE_DIR}>
)

if(BUILD_TESTING)
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
    target_link_libraries(helios-ecs-tests PRIVATE helios GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(helios-ecs-tests)
endif()

option(BUILD_BENCHMARKS "Build benchmarks" ON)
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

