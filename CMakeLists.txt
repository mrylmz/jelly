cmake_minimum_required(VERSION 3.11.1)

project(jelly CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_BUILD_TYPE Debug)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    add_definitions(-DDEBUG)
endif(${CMAKE_BUILD_TYPE} STREQUAL Debug)

include(LLVM)
include(GoogleTest)
include(Benchmark)
include(CodeCoverage)

include_directories(include PUBLIC)

file(GLOB JELLY_BASIC_HEADER_FILES     "include/Basic/*.h")
file(GLOB JELLY_BASIC_SOURCE_FILES     "lib/Basic/*.cpp")
file(GLOB JELLY_AST_HEADER_FILES       "include/AST/*.h")
file(GLOB JELLY_AST_SOURCE_FILES       "lib/AST/*.cpp")
file(GLOB JELLY_HEADER_FILES           "include/*.h")
file(GLOB JELLY_SOURCE_FILES           "src/*.cpp")
file(GLOB JELLY_CORE_HEADER_FILES      "include/Core/*.h")
file(GLOB JELLY_CORE_SOURCE_FILES      "lib/*.cpp")
file(GLOB JELLY_TEST_HEADER_FILES      "test/*.h")
file(GLOB JELLY_TEST_SOURCE_FILES      "test/*.cpp")
file(GLOB JELLY_BENCHMARK_SOURCE_FILES "benchmark/*.cpp")

# Reduce libraries to only required ones soon...
llvm_map_components_to_libnames(LLVM_LIBS all)

add_executable       (jelly ${JELLY_HEADER_FILES} ${JELLY_SOURCE_FILES})
target_link_libraries(jelly jellyCore)

add_library          (jellyBasic STATIC ${JELLY_BASIC_HEADER_FILES} ${JELLY_BASIC_SOURCE_FILES})
target_link_libraries(jellyBasic ${LLVM_LIBS})

add_library          (jellyAST STATIC ${JELLY_AST_HEADER_FILES} ${JELLY_AST_SOURCE_FILES})
target_link_libraries(jellyAST jellyBasic)

add_library          (jellyCore STATIC ${JELLY_CORE_HEADER_FILES} ${JELLY_CORE_SOURCE_FILES})
target_link_libraries(jellyCore jellyBasic jellyAST ${LLVM_LIBS})

add_executable       (jellyTest ${HEADER_FILES} ${JELLY_TEST_HEADER_FILES} ${JELLY_TEST_SOURCE_FILES})
target_link_libraries(jellyTest jellyBasic jellyAST jellyCore ${LLVM_LIBS} gtest_main)

add_executable       (jellyBenchmark ${HEADER_FILES} ${JELLY_BENCHMARK_SOURCE_FILES})
target_link_libraries(jellyBenchmark jellyBasic jellyCore benchmark_main)

append_coverage_compiler_flags()

set_property(
    TARGET jelly jellyBasic jellyAST jellyCore jellyTest jellyBenchmark
    PROPERTY XCODE_ATTRIBUTE_CLANG_ENABLE_CODE_COVERAGE YES
)

set_property(
    TARGET jellyBasic jellyAST jellyCore jellyTest jellyBenchmark
    PROPERTY XCODE_ATTRIBUTE_GCC_GENERATE_TEST_COVERAGE_FILES YES
)

set_property(
    TARGET jelly jellyBasic jellyAST jellyCore jellyTest jellyBenchmark
    PROPERTY XCODE_ATTRIBUTE_GCC_INSTRUMENT_PROGRAM_FLOW_ARCS YES
)

add_custom_command(TARGET jelly POST_BUILD
    COMMAND find . -name \"*.gcda\" -delete;
)

add_custom_command(TARGET jellyBasic POST_BUILD
    COMMAND find . -name \"*.gcda\" -delete;
)

add_custom_command(TARGET jellyAST POST_BUILD
    COMMAND find . -name \"*.gcda\" -delete;
)

add_custom_command(TARGET jellyCore POST_BUILD
    COMMAND find . -name \"*.gcda\" -delete;
)

add_custom_command(TARGET jellyTest POST_BUILD
    COMMAND find . -name \"*.gcda\" -delete;
)

add_custom_command(TARGET jellyBenchmark POST_BUILD
    COMMAND find . -name \"*.gcda\" -delete;
)

setup_target_for_coverage_gcovr_html(
    NAME jellyCoverage
    EXECUTABLE jellyTest -j ${PROCESSOR_COUNT}
    DEPENDENCIES
)
