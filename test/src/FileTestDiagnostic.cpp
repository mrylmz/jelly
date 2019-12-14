#include <string>
#include <dirent.h>
#include <fstream>
#include <regex>
#include <gtest/gtest.h>

#include "FileTestDiagnostic.h"

FileTestDiagnosticContext::FileTestDiagnosticContext(std::string filePath) :
filePath(filePath),
index(0) {
    ReadFileContent();
    ParseTestDiagnosticRecords();
    ParseArguments();
}

void FileTestDiagnosticContext::ReadFileContent() {
    std::fstream file;
    file.open(filePath, std::fstream::in);
    assert(file.is_open());

    fileContent = std::string(
        std::istreambuf_iterator<Char>(file),
        std::istreambuf_iterator<Char>()
    );

    file.close();
}

void FileTestDiagnosticContext::ParseTestDiagnosticRecords() {
    std::regex regex("\\/\\/\\s*expect-error:\\s*([^\n]+)", std::regex::icase);
    std::smatch matches;
    std::string searchContent = std::string(fileContent);
    while (std::regex_search(searchContent, matches, regex)) {
        assert(matches.size() == 2);
        FileTestDiagnosticRecord record;
        record.level = DiagnosticLevelError;
        record.message = matches[1].str();
        records.push_back(record);
        searchContent = matches.suffix();
    }

    regex = std::regex("\\/\\/\\s*expect-critical:\\s*([^\n]+)", std::regex::icase);
    searchContent = std::string(fileContent);
    while (std::regex_search(searchContent, matches, regex)) {
        assert(matches.size() == 2);
        FileTestDiagnosticRecord record;
        record.level = DiagnosticLevelCritical;
        record.message = matches[1].str();
        records.push_back(record);
        searchContent = matches.suffix();
    }

    regex = std::regex("\\/\\/\\s*report-error:\\s*([^\n]+)", std::regex::icase);
    searchContent = std::string(fileContent);
    while (std::regex_search(searchContent, matches, regex)) {
        assert(matches.size() == 2);
        reports.push_back(matches[1].str());
        searchContent = matches.suffix();
    }
}

void FileTestDiagnosticContext::ParseArguments() {
    std::regex regex("\\/\\/ run:[^\\S\\r\\n]*([^\n]*)[^\\S\\r\\n]*", std::regex::icase);
    std::smatch matches;
    std::string searchContent = std::string(fileContent);
    if (std::regex_search(searchContent, matches, regex)) {
        assert(matches.size() == 2);

        std::string match = matches[1].str();
        std::istringstream stream(match);
        for (std::string argument; stream >> argument;) {
            arguments.push_back(argument);
        }
    }
}

std::vector<FileTest> FileTest::ReadFromDirectory(std::string testDirectoryPath) {
    std::string filePath(__FILE__);
    std::string directoryPath(filePath.substr(0, filePath.rfind("/")));
    directoryPath.append("/../");
    directoryPath.append(testDirectoryPath);
    directoryPath.append("/");

    DIR *directory = opendir(directoryPath.c_str());
    assert(directory);

    std::vector<FileTest> suit;
    dirent *entry;
    while ((entry = readdir(directory)) != nullptr) {
        std::string name(entry->d_name);

        if (name.rfind(".jelly") != std::string::npos) {
            std::string filePath = std::string(directoryPath).append(name);
            std::string dumpRecordContent;
            std::string dumpFilePath = filePath.substr(0, filePath.find_last_of('.')) + ".dump";
            std::fstream dumpFile;
            dumpFile.open(dumpFilePath, std::fstream::in);
            bool hasDumpRecord = dumpFile.good();
            if (hasDumpRecord) {
                dumpRecordContent = std::string(std::istreambuf_iterator<Char>(dumpFile),
                                                std::istreambuf_iterator<Char>());
            }

            if (dumpFile.is_open()) {
                dumpFile.close();
            }

            FileTest test(FileTestDiagnosticContext(filePath), hasDumpRecord, filePath, name, dumpFilePath, dumpRecordContent);
            suit.push_back(test);
        }
    }

    closedir(directory);
    return suit;
}

std::string FileTest::GetFileName(std::string filepath) {
    std::string result(filepath);
    if (result.rfind(".") != std::string::npos) {
        result = result.substr(0, result.find_last_of('.'));
    }

    if (result.find("/") != std::string::npos) {
        result = result.substr(result.find_last_of('/') + 1);
    }

    return result;
}

void FileTestDiagnosticHandler(DiagnosticLevel level, const Char *message, void *context) {
    printf("[  MESSAGE ] %s\n", message);

    FileTestDiagnosticContext *fileTestContext = (FileTestDiagnosticContext *)context;
    if (fileTestContext->records.size() <= fileTestContext->index) {
        FAIL();
    }

    FileTestDiagnosticRecord record = fileTestContext->records[fileTestContext->index];
    EXPECT_EQ(record.level, level);
    EXPECT_STREQ(record.message.c_str(), message);
    fileTestContext->index += 1;
}
