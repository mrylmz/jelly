#include <gtest/gtest.h>
#include <JellyCore/JellyCore.h>

#include "FileTestDiagnostic.h"

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    for (int i = 0; i < 100; i++) {
        RUN_ALL_TESTS();
    }
    return RUN_ALL_TESTS();
}
