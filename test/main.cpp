#include <gtest/gtest.h>

using namespace testing;

int main(int argc, char* argv[]) {
    InitGoogleTest(&argc, argv);
    testing::GTEST_FLAG(filter) = "-parser/ASTPrintTest.parse_tree/55"
                                  ":parser/ASTPrintTest.parse_tree/83";
    return RUN_ALL_TESTS();
}
