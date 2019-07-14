#include <gtest/gtest.h>
#include <JellyCore/JellyCore.h>
#include <string>
#include <dirent.h>
#include <fstream>
#include <regex>

#include "FileTestDiagnostic.h"

class TypeCheckerTests : public testing::TestWithParam<FileTest> {
};

TEST_P(TypeCheckerTests, run) {
    auto test = GetParam();
    printf("[   TEST   ] %s\n", test.context.filePath.substr(test.context.filePath.rfind("/")).c_str());

    if (!test.context.reports.empty()) {
        for (auto error : test.context.reports) {
            printf("[  FAILED  ] %s!\n", error.c_str());
        }

        FAIL();
    } else {
        DiagnosticEngineSetDefaultHandler(&FileTestDiagnosticHandler, &test.context);

        std::string filename = test.GetFileName(test.filePath);
        
        StringRef absoluteFilePath = StringCreate(AllocatorGetSystemDefault(), test.context.filePath.c_str());
        StringRef workingDirectory = StringCreateCopyUntilLastOccurenceOf(AllocatorGetSystemDefault(), absoluteFilePath, '/');

        StringRef executable = StringCreate(AllocatorGetSystemDefault(), "jelly");
        StringRef filePath = StringCreateCopyFromLastOccurenceOf(AllocatorGetSystemDefault(), absoluteFilePath, '/');
        StringRef workingDirectoryArgument = StringCreate(AllocatorGetSystemDefault(), "-working-directory=");
        StringAppendString(workingDirectoryArgument, workingDirectory);
        StringRef moduleName = StringCreate(AllocatorGetSystemDefault(), filename.c_str());
        StringRef moduleNameArgument = StringCreate(AllocatorGetSystemDefault(), "-module-name=");
        StringAppendString(moduleNameArgument, moduleName);

        ArrayRef arguments = ArrayCreateEmpty(AllocatorGetSystemDefault(), sizeof(StringRef), 4);
        ArrayAppendElement(arguments, &executable);
        ArrayAppendElement(arguments, &filePath);
        ArrayAppendElement(arguments, &workingDirectoryArgument);
        ArrayAppendElement(arguments, &moduleNameArgument);

        CompilerRun(arguments);

        for (Index index = 0; index < ArrayGetElementCount(arguments); index++) {
            StringDestroy(*((StringRef*)ArrayGetElementAtIndex(arguments, index)));
        }

        StringDestroy(moduleName);
        ArrayDestroy(arguments);

        if (test.context.index < test.context.records.size()) {
            for (auto index = test.context.index; index < test.context.records.size(); index++) {
                printf("[ EXPECTED ] %s\n", test.context.records[index].message.c_str());
            }

            FAIL();
        }
    }
}

INSTANTIATE_TEST_CASE_P(run, TypeCheckerTests, testing::ValuesIn(FileTest::ReadFromDirectory("sema")));
