cmake_minimum_required(VERSION 3.11.1)

project(jelly)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include PUBLIC)

file(GLOB HEADER_FILES
    "include/jelly/*.h"
)

file(GLOB SOURCE_FILES
    "src/*.cpp"
)

add_executable(jelly 
    ${HEADER_FILES}
    ${SOURCE_FILES}
)
