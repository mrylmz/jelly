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

#include <dirent.h>
#include <fstream>
#include <regex>

#include <gtest/gtest.h>

FileTestDiagnosticHandler::FileTestDiagnosticHandler(FileTestMetadata metadata) : metadata(metadata) {}

void FileTestDiagnosticHandler::begin(jelly::SourceBuffer* buffer) {
    index = 0;
}

void FileTestDiagnosticHandler::end() {
    EXPECT_EQ(index, metadata.errorMessages.size());
}

void FileTestDiagnosticHandler::finish() {

}

void FileTestDiagnosticHandler::handle(jelly::Diagnostic diagnostic) {
    printf("[  MESSAGE ] %s\n", diagnostic.getMessage().c_str());

    if (metadata.errorMessages.size() <= index) {
        FAIL();
    }

    EXPECT_STREQ(diagnostic.getMessage().c_str(), metadata.errorMessages[index].c_str());
    index += 1;

}

bool readFileContent(std::string filePath, std::string& fileContent) {
    std::fstream file;
    file.open(filePath, std::fstream::in);

    bool is_open = file.is_open();
    if (is_open) {
        fileContent = {
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        };
    }

    file.close();
    return is_open;
}

bool writeFileContent(std::string filePath, std::string fileContent) {
    std::fstream file;
    file.open(filePath, std::fstream::out);

    bool is_open = file.is_open();
    if (is_open) {
        file.write(fileContent.c_str(), fileContent.length());
    }

    file.close();
    return is_open;
}

std::string getTestDirectoryPath() {
    std::string testFilePath = __FILE__;
    std::string testDirectory = testFilePath.substr(0, testFilePath.rfind("/"));
    return testDirectory;
}

std::vector<FileTestMetadata> readFileTestMetadataFromTestSubdirectory(std::string subdirectory, std::string fileExtension) {
    std::string workingDirectory = getTestDirectoryPath().append("/").append(subdirectory).append("/");
    std::vector<FileTestMetadata> metadata;
    DIR* directory = opendir(workingDirectory.c_str());
    if (directory) {
        dirent* directoryEntry;
        while ((directoryEntry = readdir(directory)) != nullptr) {
            std::string name(directoryEntry->d_name);

            if (name.rfind(fileExtension) != std::string::npos) {
                FileTestMetadata data;
                data.workingDirectory = workingDirectory;
                data.sourceFileName = name;
                data.sourceFilePath = std::string(workingDirectory).append(name);
                metadata.push_back(data);
            }
        }

        closedir(directory);
    }

    for (auto &data : metadata) {
        if (readFileContent(data.sourceFilePath, data.sourceFileContent)) {
            std::regex regex("\\/\\/\\s*expect-error:\\s*([^\n]+)", std::regex::icase);
            std::smatch matches;

            std::string searchContent = std::string(data.sourceFileContent);
            while (std::regex_search(searchContent, matches, regex)) {
                assert(matches.size() == 2);
                data.errorMessages.push_back(matches[1]);
                searchContent = matches.suffix();
            }

            regex = std::regex("\\/\\/\\s*report-error:\\s*([^\n]+)", std::regex::icase);
            searchContent = std::string(data.sourceFileContent);
            while (std::regex_search(searchContent, matches, regex)) {
                assert(matches.size() == 2);
                data.errorReports.push_back(matches[1]);
                searchContent = matches.suffix();
            }
        } else {
            data.errorReports.push_back("Couldn't read source file content!");
        }
    }

    return metadata;
}
