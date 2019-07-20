#include "JellyCore/Diagnostic.h"
#include "JellyCore/LDLinker.h"

// TODO: Embed lld into project instead of calling system!

void LDLinkerLink(AllocatorRef allocator, ArrayRef objectFiles, StringRef targetPath, LDLinkerTargetType targetType,
                  StringRef architecture) {
    StringRef command = StringCreate(allocator, "ld");
    for (Index index = 0; index < ArrayGetElementCount(objectFiles); index++) {
        StringRef objectFile = *((StringRef *)ArrayGetElementAtIndex(objectFiles, index));
        StringAppendFormat(command, " %s", StringGetCharacters(objectFile));
    }

    switch (targetType) {
    case LDLinkerTargetTypeExecutable:
        StringAppendFormat(command, " %s", "-execute");
        break;
    case LDLinkerTargetTypeDylib:
        StringAppendFormat(command, " %s", "-dylib");
        break;
    case LDLinkerTargetTypeBundle:
        StringAppendFormat(command, " %s", "-bundle");
        break;
    case LDLinkerTargetTypeStatic:
        StringAppendFormat(command, " %s", "-static");
        break;
    }

    if (architecture) {
        StringAppendFormat(command, " -arch %s", StringGetCharacters(architecture));
    }

    if (targetPath) {
        StringAppendFormat(command, " -o %s", StringGetCharacters(targetPath));
    }

    // dynamic main executables must link with libSystem.dylib for inferred architecture x86_64
    StringAppendFormat(command, " %s", "-lSystem");

    Int status = system(StringGetCharacters(command));
    if (status != EXIT_SUCCESS) {
        ReportError("Linking process failed!");
    }

    StringDestroy(command);
}
