cmake_minimum_required(VERSION 3.11.1)

project(jelly
        VERSION 0.0.0
        LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(JELLY_CORE_HEADER_FILES "include/JellyCore/Allocator.h"
                            "include/JellyCore/Array.h"
                            "include/JellyCore/Base.h"
                            "include/JellyCore/JellyCore.h"
                            "include/JellyCore/String.h")
set(JELLY_CORE_SOURCE_FILES "lib/JellyCore/Allocator.c"
                            "lib/JellyCore/Array.c"
                            "lib/JellyCore/String.c")
add_library(JellyCore STATIC ${JELLY_CORE_HEADER_FILES} 
                             ${JELLY_CORE_SOURCE_FILES})
target_include_directories(JellyCore PUBLIC include)

add_executable(${PROJECT_NAME} "src/main.c")
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} JellyCore)