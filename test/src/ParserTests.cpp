#include <gtest/gtest.h>
#include <JellyCore/JellyCore.h>
#include <string>
#include <dirent.h>
#include <fstream>
#include <regex>

#include "FileTestDiagnostic.h"

static inline std::string ReadFileContent(std::string filePath) {
    std::fstream file;
    file.open(filePath, std::fstream::in);
    assert(file.is_open());
    std::string fileContent = std::string(std::istreambuf_iterator<Char>(file), std::istreambuf_iterator<Char>());
    file.close();
    return fileContent;
}

static inline void WriteFileContent(std::string filePath, std::string content) {
    std::fstream file;
    file.open(filePath, std::fstream::out);
    assert(file.is_open());
    file.write(content.c_str(), content.length());
    file.close();
}

class ParserTest : public testing::TestWithParam<FileTest> {
};

TEST_P(ParserTest, run) {
    auto test = GetParam();
    printf("[   TEST   ] %s\n", test.context.filePath.substr(test.context.filePath.rfind("/")).c_str());

    if (!test.context.reports.empty()) {
        for (auto error : test.context.reports) {
            printf("[  FAILED  ] %s!\n", error.c_str());
        }

        FAIL();
    } else {
        DiagnosticEngineSetDefaultHandler(&FileTestDiagnosticHandler, &test.context);

        const Char *dumpFileName = "temp.dump";
        remove(dumpFileName);

        StringRef absoluteFilePath = StringCreate(AllocatorGetSystemDefault(), test.context.filePath.c_str());
        StringRef workingDirectory = StringCreateCopyUntilLastOccurenceOf(AllocatorGetSystemDefault(), absoluteFilePath, '/');

        StringRef executable = StringCreate(AllocatorGetSystemDefault(), "jelly");
        StringRef filePath = StringCreateCopyFromLastOccurenceOf(AllocatorGetSystemDefault(), absoluteFilePath, '/');
        StringRef argumentDumpAST = StringCreate(AllocatorGetSystemDefault(), "-dump-ast=");
        StringAppend(argumentDumpAST, dumpFileName);
        StringRef workingDirectoryArgument = StringCreate(AllocatorGetSystemDefault(), "-working-directory=");
        StringAppendString(workingDirectoryArgument, workingDirectory);

        ArrayRef arguments = ArrayCreateEmpty(AllocatorGetSystemDefault(), sizeof(StringRef), 4);
        ArrayAppendElement(arguments, &executable);
        ArrayAppendElement(arguments, &filePath);
        ArrayAppendElement(arguments, &argumentDumpAST);
        ArrayAppendElement(arguments, &workingDirectoryArgument);

        CompilerRun(arguments);

        for (Index index = 0; index < ArrayGetElementCount(arguments); index++) {
            StringDestroy(*((StringRef*)ArrayGetElementAtIndex(arguments, index)));
        }

        ArrayDestroy(arguments);

        std::string dumpContent = ReadFileContent(dumpFileName);
        if (test.context.index < test.context.records.size()) {
            for (auto index = test.context.index; index < test.context.records.size(); index++) {
                printf("[ EXPECTED ] %s\n", test.context.records[index].message.c_str());
            }

            FAIL();
        }

        if (test.hasDumpRecord) {
            printf("[ RUN      ] %s\n", test.relativeFilePath.c_str());
            EXPECT_STREQ(test.dumpRecordContent.c_str(), dumpContent.c_str());
        } else {
            printf("[ REC      ] %s\n", test.relativeFilePath.c_str());
            WriteFileContent(test.dumpFilePath, dumpContent.c_str());
            printf("[       OK ] %s\n", test.relativeFilePath.c_str());
        }
    }
}

INSTANTIATE_TEST_CASE_P(run, ParserTest, testing::ValuesIn(FileTest::ReadFromDirectory("parser")));
