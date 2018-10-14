cmake_minimum_required(VERSION 3.11.1)

project(jelly CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_BUILD_TYPE Debug)

include_directories(include PUBLIC)

file(GLOB JELLY_HEADER_FILES    "include/*.h")
file(GLOB JELLY_SOURCE_FILES    "src/*.cpp")

add_executable       (jelly ${JELLY_HEADER_FILES} ${JELLY_SOURCE_FILES})
target_link_libraries(jelly jellyBasic jellySyntax jellyParse)

file(GLOB AST_HEADER_FILES    "include/AST/*.h")
file(GLOB AST_SOURCE_FILES    "lib/AST/*.cpp")

add_library           (jellyAST STATIC ${AST_HEADER_FILES} ${AST_SOURCE_FILES})
target_link_libraries (jellyAST jellyBasic)

file(GLOB BASIC_HEADER_FILES    "include/Basic/*.h")
file(GLOB BASIC_SOURCE_FILES    "lib/Basic/*.cpp")

add_library(jellyBasic STATIC ${BASIC_HEADER_FILES} ${BASIC_SOURCE_FILES})

file(GLOB SYNTAX_HEADER_FILES   "include/Syntax/*.h")
file(GLOB SYNTAX_SOURCE_FILES   "lib/Syntax/*.cpp")

add_library           (jellySyntax STATIC ${SYNTAX_HEADER_FILES} ${SYNTAX_SOURCE_FILES})
target_link_libraries (jellySyntax jellyBasic)

file(GLOB PARSE_HEADER_FILES    "include/Parse/*.h")
file(GLOB PARSE_SOURCE_FILES    "lib/Parse/*.cpp")

add_library           (jellyParse STATIC ${PARSE_HEADER_FILES} ${PARSE_SOURCE_FILES})
target_link_libraries (jellyParse jellyBasic jellySyntax jellyAST)

file(GLOB TEST_SOURCE_FILES     "test/*.cpp")

include(GoogleTest)

add_executable(jellyTest 
    ${HEADER_FILES}
    ${TEST_SOURCE_FILES}
)

target_link_libraries(jellyTest
    jellyBasic
    jellySyntax
    jellyAST
    jellyParse
    gtest_main
)

include(Benchmark)

file(GLOB BENCHMARK_SOURCE_FILES     "benchmark/*.cpp")

add_executable(jellyBenchmark 
    ${HEADER_FILES}
    ${BENCHMARK_SOURCE_FILES}
)

target_link_libraries(jellyBenchmark
    jellyBasic
    jellySyntax
    jellyAST
    jellyParse
    benchmark_main
)

include(CodeCoverage)

APPEND_COVERAGE_COMPILER_FLAGS()

set_property(
    TARGET jellyBasic jellySyntax jellyParse jellyTest jellyBenchmark
    PROPERTY XCODE_ATTRIBUTE_CLANG_ENABLE_CODE_COVERAGE YES
)

set_property(
    TARGET jellyBasic jellySyntax jellyParse jellyTest jellyBenchmark
    PROPERTY XCODE_ATTRIBUTE_GCC_GENERATE_TEST_COVERAGE_FILES YES
)

set_property(
    TARGET jellyBasic jellySyntax jellyParse jellyTest jellyBenchmark
    PROPERTY XCODE_ATTRIBUTE_GCC_INSTRUMENT_PROGRAM_FLOW_ARCS YES
)

add_custom_command(TARGET jellyTest POST_BUILD
    COMMAND find . -name \"*.gcda\" -delete;
)

add_custom_command(TARGET jellyBenchmark POST_BUILD
    COMMAND find . -name \"*.gcda\" -delete;
)

SETUP_TARGET_FOR_COVERAGE_GCOVR_HTML(
    NAME jellyCoverage
    EXECUTABLE jellyTest -j ${PROCESSOR_COUNT}
    DEPENDENCIES
)
