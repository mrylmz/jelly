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

#pragma once

#include <Core/Core.h>

#include <string>
#include <vector>

struct FileTestMetadata {
    std::string workingDirectory;
    std::string sourceFileName;
    std::string sourceFilePath;
    std::string sourceFileContent;
    std::vector<std::string> errorMessages;
    std::vector<std::string> errorReports;
};

struct FileTestDiagnosticHandler : public jelly::DiagnosticHandler {
    FileTestMetadata metadata;
    unsigned index;

    FileTestDiagnosticHandler(FileTestMetadata metadata);

    void begin(jelly::SourceBuffer buffer);

    void end();

    void finish();

    void handle(jelly::Diagnostic diagnostic);
};


std::string getTestDirectoryPath();

bool readFileContent(std::string filePath, std::string& fileContent);
bool writeFileContent(std::string filePath, std::string fileContent);

std::vector<FileTestMetadata> readFileTestMetadataFromTestSubdirectory(std::string subdirectory, std::string fileExtension);
