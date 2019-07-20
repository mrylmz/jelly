#ifndef __JELLYTEST_FILETESTDIAGNOSTIC__
#define __JELLYTEST_FILETESTDIAGNOSTIC__

#include <JellyCore/JellyCore.h>

struct FileTestDiagnosticRecord {
    DiagnosticLevel level;
    std::string message;
};

struct FileTestDiagnosticContext {
    std::string filePath;
    std::string fileContent;
    Index index;
    std::vector<FileTestDiagnosticRecord> records;
    std::vector<std::string> reports;
    std::vector<std::string> arguments;

    FileTestDiagnosticContext(std::string filePath);

  private:
    void ReadFileContent();
    void ParseTestDiagnosticRecords();
    void ParseArguments();
};

struct FileTest {
    FileTestDiagnosticContext context;
    Bool hasDumpRecord;
    std::string filePath;
    std::string relativeFilePath;
    std::string dumpFilePath;
    std::string dumpRecordContent;

    FileTest(FileTestDiagnosticContext context, Bool hasDumpRecord, std::string filePath, std::string relativeFilePath,
             std::string dumpFilePath, std::string dumpRecordContent)
        : context(context), hasDumpRecord(hasDumpRecord), filePath(filePath), relativeFilePath(relativeFilePath),
          dumpFilePath(dumpFilePath), dumpRecordContent(dumpRecordContent) {
    }

    static std::vector<FileTest> ReadFromDirectory(std::string testDirectoryPath);

    static std::string GetFileName(std::string filepath);
};

JELLY_EXTERN_C_BEGIN

void FileTestDiagnosticHandler(DiagnosticLevel level, const Char *message, void *context);

JELLY_EXTERN_C_END

#endif
