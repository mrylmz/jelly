#include <gtest/gtest.h>
#include <JellyCore/JellyCore.h>

#include "FileTestDiagnostic.h"

#warning TODO: Add IRBuilder tests and apply diff on IR code for correctness!

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
