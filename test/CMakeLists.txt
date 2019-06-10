file(GLOB JELLY_TEST_HEADER_FILES include/*.h)
file(GLOB JELLY_TEST_SOURCE_FILES src/*.cpp)

add_executable(JellyTest ${JELLY_TEST_HEADER_FILES} ${JELLY_TEST_SOURCE_FILES})

target_include_directories(JellyTest PUBLIC ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
target_include_directories(JellyTest PUBLIC include)
target_link_libraries(JellyTest JellyCore gtest gtest_main)

enable_testing()
add_test(JellyTest JellyTest)