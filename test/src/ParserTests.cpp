//
// MIT License
//
// Copyright (c) 2018 Murat Yilmaz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <gtest/gtest.h>
#include <JellyCore/JellyCore.h>
#include <string>
#include <dirent.h>
#include <fstream>
#include <regex>

#include "FileTestDiagnostic.h"

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

        Char dumpBuffer[65535];
        FILE *dumpStream = fmemopen(dumpBuffer, sizeof(dumpBuffer) / sizeof(Char), "w");
        assert(dumpStream);

        AllocatorRef allocator = AllocatorGetSystemDefault();
        ASTContextRef context = ASTContextCreate(allocator);
        ASTDumperRef dumper = ASTDumperCreate(allocator, dumpStream);
        ParserRef parser = ParserCreate(allocator, context);
        StringRef filePath = StringCreate(allocator, test.context.filePath.c_str());
        StringRef relativeFilePath = StringCreate(allocator, test.relativeFilePath.c_str());
        StringRef source = StringCreateFromFile(allocator, StringGetCharacters(filePath));
        ParserParseSourceUnit(parser, relativeFilePath, source);
        ASTDumperDump(dumper, (ASTNodeRef)ASTContextGetModule(context));
        StringDestroy(source);
        StringDestroy(relativeFilePath);
        StringDestroy(filePath);
        ParserDestroy(parser);
        ASTDumperDestroy(dumper);
        ASTContextDestroy(context);

        fclose(dumpStream);

        if (test.context.index < test.context.records.size()) {
            for (auto index = test.context.index; index < test.context.records.size(); index++) {
                printf("[ EXPECTED ] %s!\n", test.context.records[index].message.c_str());
            }

            FAIL();
        }

        if (test.hasDumpRecord) {
            printf("[ RUN      ] %s\n", test.relativeFilePath.c_str());
            EXPECT_STREQ(test.dumpRecordContent.c_str(), dumpBuffer);
        } else {
            printf("[ REC      ] %s\n", test.relativeFilePath.c_str());
            WriteFileContent(test.dumpFilePath, dumpBuffer);
            printf("[       OK ] %s\n", test.relativeFilePath.c_str());
        }
    }
}

INSTANTIATE_TEST_CASE_P(run, ParserTest, testing::ValuesIn(FileTest::ReadFromDirectory("parser")));
