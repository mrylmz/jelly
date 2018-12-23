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

#include "Util.h"

#include <Core/Core.h>
#include <gtest/gtest.h>

struct ParserTestData {
    FileTestMetadata metadata;
    bool hasDumpRecord;
    std::string dumpRecordFilePath;
    std::string dumpRecordFileContent;
};

static std::vector<ParserTestData> readParseTestDataFromTestSubdirectory(std::string subdirectory, std::string fileExtension) {
    auto metadata = readFileTestMetadataFromTestSubdirectory(subdirectory, fileExtension);

    std::vector<ParserTestData> result;
    for (auto metadata : metadata) {
        ParserTestData data;
        data.metadata = metadata;
        data.dumpRecordFilePath = metadata.sourceFilePath;

        auto pos = data.dumpRecordFilePath.rfind(fileExtension);
        if (pos != std::string::npos) {
            data.dumpRecordFilePath.replace(pos, data.dumpRecordFilePath.length() - pos, ".dump");
        } else {
            data.dumpRecordFilePath.append(".dump");
        }

        data.hasDumpRecord = readFileContent(data.dumpRecordFilePath, data.dumpRecordFileContent);
        result.push_back(data);
    }

    return result;
}

class ParserTest : public testing::TestWithParam<ParserTestData> {
};

TEST_P(ParserTest, run) {
    auto parameter = GetParam();
    printf("[   TEST   ] %s\n", parameter.metadata.sourceFileName.c_str());

    if (!parameter.metadata.errorReports.empty()) {
        for (auto error : parameter.metadata.errorReports) {
            printf("[  FAILED  ] %s!\n", error.c_str());
        }

        FAIL();
    } else {
        FileTestDiagnosticHandler diagHandler(parameter.metadata);
        CodeManager manager(parameter.metadata.workingDirectory, &diagHandler);
        manager.addSourceText(parameter.metadata.sourceFileContent);
        manager.parseAST();

        std::ostringstream dumpRecordContentStream;
        ASTDumper dumper(dumpRecordContentStream);
        dumper.dumpNode(manager.context.getRoot());

        if (parameter.hasDumpRecord) {
            printf("[ RUN      ] %s\n", parameter.metadata.sourceFileName.c_str());

            EXPECT_STREQ(parameter.dumpRecordFileContent.c_str(), dumpRecordContentStream.str().c_str());
        } else {
            printf("[ REC      ] %s\n", parameter.metadata.sourceFileName.c_str());

            if (!writeFileContent(parameter.dumpRecordFilePath, dumpRecordContentStream.str())) {
                printf("[  FAILED  ] %s\n", parameter.metadata.sourceFileName.c_str());
                FAIL();
            } else {
                printf("[       OK ] %s\n", parameter.metadata.sourceFileName.c_str());
            }
        }
    }
}

INSTANTIATE_TEST_CASE_P(run, ParserTest, testing::ValuesIn(readParseTestDataFromTestSubdirectory("parser", ".jelly")));
