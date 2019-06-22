#include <gtest/gtest.h>
#include <JellyCore/JellyCore.h>
#include <string>
#include <dirent.h>
#include <fstream>
#include <regex>

#include "FileTestDiagnostic.h"

class NameBindingTest : public testing::TestWithParam<FileTest> {
};

TEST_P(NameBindingTest, run) {
    auto test = GetParam();
    printf("[   TEST   ] %s\n", test.context.filePath.substr(test.context.filePath.rfind("/")).c_str());

    if (!test.context.reports.empty()) {
        for (auto error : test.context.reports) {
            printf("[  FAILED  ] %s!\n", error.c_str());
        }

        FAIL();
    } else {
        DiagnosticEngineSetDefaultHandler(&FileTestDiagnosticHandler, &test.context);

        Char dumpBuffer[65535];
        FILE *dumpStream = fmemopen(dumpBuffer, sizeof(dumpBuffer) / sizeof(Char), "w");
        assert(dumpStream);

        AllocatorRef allocator = AllocatorGetSystemDefault();
        StringRef absoluteFilePath = StringCreate(allocator, test.context.filePath.c_str());
        StringRef workingDirectory = StringCreateCopyUntilLastOccurenceOf(allocator, absoluteFilePath, '/');
        StringRef filePath = StringCreateCopyFromLastOccurenceOf(allocator, absoluteFilePath, '/');
        WorkspaceRef workspace = WorkspaceCreate(allocator, workingDirectory);
        ASTContextRef context = WorkspaceGetContext(workspace);
        WorkspaceAddSourceFile(workspace, filePath);
        WorkspaceStartAsync(workspace);
        WorkspaceWaitForFinish(workspace);
        NameResolverRef resolver = NameResolverCreate(allocator);
        NameResolverResolve(resolver, context, (ASTNodeRef)ASTContextGetModule(context));
        NameResolverDestroy(resolver);
        WorkspaceDestroy(workspace);
        StringDestroy(workingDirectory);
        StringDestroy(filePath);
        StringDestroy(absoluteFilePath);

        fclose(dumpStream);

        if (test.context.index < test.context.records.size()) {
            for (auto index = test.context.index; index < test.context.records.size(); index++) {
                printf("[ EXPECTED ] %s\n", test.context.records[index].message.c_str());
            }

            FAIL();
        }
    }
}

INSTANTIATE_TEST_CASE_P(run, NameBindingTest, testing::ValuesIn(FileTest::ReadFromDirectory("name_resolution")));
