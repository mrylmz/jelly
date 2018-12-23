#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    testing::GTEST_FLAG(filter) =
    "-run/ParserTest.run/54"
    ":run/ParserTest.run/82"
    ":run/SemaTest.run/5"
    ":run/SemaTest.run/19";

    return RUN_ALL_TESTS();
}
