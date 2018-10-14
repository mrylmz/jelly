cmake_minimum_required (VERSION 2.8.12)

set(BENCHMARK_ENABLE_TESTING OFF)

configure_file(cmake/Benchmark.ink.txt benchmark-download/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/benchmark-download )
if(result)
    message(FATAL_ERROR "CMake step for benchmark failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/benchmark-download )
if(result)
    message(FATAL_ERROR "Build step for benchmark failed: ${result}")
endif()

add_subdirectory(${CMAKE_BINARY_DIR}/benchmark-src
                 ${CMAKE_BINARY_DIR}/benchmark-build
                 EXCLUDE_FROM_ALL)