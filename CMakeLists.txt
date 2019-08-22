cmake_minimum_required(VERSION 3.11.1)

project(jelly
        VERSION 0.1.0
        LANGUAGES C)

if (APPLE)
    # Add support for brew packages which will not be linked by
    # brew because they are expected to be pre-installed on the
    # target os (e.g. llvm on macOS)
    list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG=1")

# LLVM Libraries
include(LLVM)

# All llvm libraries will be linked for now should be reduced to the required ones soon...
llvm_map_components_to_libnames(JELLY_LLVM_LIBS all)

# Libraries
set(JELLY_CORE_HEADER_FILES "include/JellyCore/Allocator.h"
                            "include/JellyCore/Array.h"
                            "include/JellyCore/ASTArray.h"
                            "include/JellyCore/ASTContext.h"
                            "include/JellyCore/ASTDumper.h"
                            "include/JellyCore/ASTFunctions.h"
                            "include/JellyCore/ASTMangling.h"
                            "include/JellyCore/ASTNodes.h"
                            "include/JellyCore/ASTScope.h"
                            "include/JellyCore/ASTSubstitution.h"
                            "include/JellyCore/Base.h"
                            "include/JellyCore/BumpAllocator.h"
                            "include/JellyCore/Compiler.h"
                            "include/JellyCore/Diagnostic.h"
                            "include/JellyCore/IRBuilder.h"
                            "include/JellyCore/JellyCore.h"
                            "include/JellyCore/LDLinker.h"
                            "include/JellyCore/Lexer.h"
                            "include/JellyCore/NameResolution.h"
                            "include/JellyCore/Parser.h"
                            "include/JellyCore/Queue.h"
                            "include/JellyCore/RuntimeSupportDefinitions.h"
                            "include/JellyCore/SourceRange.h"
                            "include/JellyCore/String.h"
                            "include/JellyCore/TypeChecker.h"
                            "include/JellyCore/Workspace.h")
set(JELLY_CORE_SOURCE_FILES "lib/JellyCore/Allocator.c"
                            "lib/JellyCore/Array.c"
                            "lib/JellyCore/ASTArray.c"
                            "lib/JellyCore/ASTContext.c"
                            "lib/JellyCore/ASTDumper.c"
                            "lib/JellyCore/ASTFunctions.c"
                            "lib/JellyCore/ASTMangling.c"
                            "lib/JellyCore/ASTScope.c"
                            "lib/JellyCore/ASTSubstitution.c"
                            "lib/JellyCore/BumpAllocator.c"
                            "lib/JellyCore/Compiler.c"
                            "lib/JellyCore/Diagnostic.c"
                            "lib/JellyCore/IRBuilder.c"
                            "lib/JellyCore/LDLinker.c"
                            "lib/JellyCore/Lexer.c"
                            "lib/JellyCore/NameResolution.c"
                            "lib/JellyCore/Parser.c"
                            "lib/JellyCore/Queue.c"
                            "lib/JellyCore/SourceRange.c"
                            "lib/JellyCore/String.c"
                            "lib/JellyCore/TypeChecker.c"
                            "lib/JellyCore/Workspace.c")
add_library(JellyCore STATIC ${JELLY_CORE_HEADER_FILES} 
                             ${JELLY_CORE_SOURCE_FILES})
target_include_directories(JellyCore PUBLIC include)
target_link_libraries(JellyCore ${JELLY_LLVM_LIBS})

# Tools
add_executable(${PROJECT_NAME} "src/main.cpp")
target_compile_definitions(${PROJECT_NAME}
                           PRIVATE -DJELLY_VERSION="${PROJECT_VERSION}")
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} JellyCore)

# Testing
add_subdirectory(googletest)
add_subdirectory(test)

# Code Coverage
include(CodeCoverage)
append_coverage_compiler_flags()

set_target_properties(
    JellyCore 
    PROPERTIES 
    GCC_INSTRUMENT_PROGRAM_FLOW_ARCS YES 
    GCC_GENERATE_TEST_COVERAGE_FILES YES
)

set_target_properties(
    JellyTest 
    PROPERTIES 
    GCC_INSTRUMENT_PROGRAM_FLOW_ARCS YES 
    GCC_GENERATE_TEST_COVERAGE_FILES NO
)

setup_target_for_coverage_gcovr_html(
    NAME JellyCoverage
    EXECUTABLE JellyTest -j ${PROCESSOR_COUNT}
    DEPENDENCIES
)
