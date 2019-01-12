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

class SemaTest : public testing::TestWithParam<FileTestMetadata> {
};

TEST_P(SemaTest, run) {
    auto parameter = GetParam();

    SourceManager::setWorkingDirectory(parameter.workingDirectory);

    printf("[   TEST   ] %s\n", parameter.sourceFileName.c_str());

    if (!parameter.errorReports.empty()) {
        for (auto error : parameter.errorReports) {
            printf("[  FAILED  ] %s!\n", error.c_str());
        }

        FAIL();
    } else {
        FileTestDiagnosticHandler diagHandler(parameter);
        CodeManager manager(&diagHandler);
        manager.addSourceText(parameter.sourceFileContent);
        manager.typecheckAST();
    }
}

INSTANTIATE_TEST_CASE_P(run, SemaTest, testing::ValuesIn(readFileTestMetadataFromTestSubdirectory("sema", ".jelly")));
