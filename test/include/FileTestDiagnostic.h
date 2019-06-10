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

    FileTestDiagnosticContext(std::string filePath);

  private:
    void ReadFileContent();
    void ParseTestDiagnosticRecords();
};

struct FileTest {
    FileTestDiagnosticContext context;
    Bool hasDumpRecord;

    FileTest(FileTestDiagnosticContext context, Bool hasDumpRecord) : context(context), hasDumpRecord(hasDumpRecord) {}

    static std::vector<FileTest> ReadFromDirectory(std::string testDirectoryPath);
};

JELLY_EXTERN_C_BEGIN

void FileTestDiagnosticHandler(DiagnosticLevel level, const Char *message, void *context);

JELLY_EXTERN_C_END

#endif
