include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.4
)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
)

set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark glm)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(TARGET benchmark)
        target_compile_options(benchmark PRIVATE -Wno-c2y-extensions)
    endif()
    if(TARGET benchmark_main)
        target_compile_options(benchmark_main PRIVATE -Wno-c2y-extensions)
    endif()
endif()


file(GLOB_RECURSE BENCHMARK_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.benchmark.cpp")

foreach(BENCHMARK_FILE ${BENCHMARK_FILES})
    file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${BENCHMARK_FILE})
    get_filename_component(SUBDIR ${REL_PATH} DIRECTORY)
    get_filename_component(TARGET_NAME ${BENCHMARK_FILE} NAME_WE)

    string(REPLACE "/" "" UNIQUE_TARGET ${REL_PATH})
    string(REPLACE ".benchmark.cpp" "" UNIQUE_TARGET ${UNIQUE_TARGET})

    add_executable(${UNIQUE_TARGET} ${BENCHMARK_FILE})

    target_link_libraries(${UNIQUE_TARGET} PRIVATE
            helios
            benchmark::benchmark
            glm::glm
    )

    set_target_properties(${UNIQUE_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks/${SUBDIR}
        OUTPUT_NAME ${TARGET_NAME}
    )
endforeach()


