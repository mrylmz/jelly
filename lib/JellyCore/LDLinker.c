#include "JellyCore/Diagnostic.h"
#include "JellyCore/LDLinker.h"

// TODO: Embed lld into project instead of calling system!

void LDLinkerLink(AllocatorRef allocator, ArrayRef objectFiles, ArrayRef linkLibraries, ArrayRef linkFrameworks, StringRef targetPath,
                  LDLinkerTargetType targetType, StringRef architecture) {
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
        ReportCritical("Static libraries are currently not supported!");
        return;
    }

    if (architecture) {
        StringAppendFormat(command, " -arch %s", StringGetCharacters(architecture));
    }

    if (targetPath) {
        StringAppendFormat(command, " -o %s", StringGetCharacters(targetPath));
    }

    for (Index index = 0; index < ArrayGetElementCount(linkLibraries); index++) {
        StringRef linkLibrary = *((StringRef *)ArrayGetElementAtIndex(linkLibraries, index));
        StringAppendFormat(command, " %s", "-l");

        // TODO: The linkLibrary string should be validated and escaped for security!!!
        StringAppendString(command, linkLibrary);
    }

    for (Index index = 0; index < ArrayGetElementCount(linkFrameworks); index++) {
        StringRef linkFramework = *((StringRef *)ArrayGetElementAtIndex(linkFrameworks, index));

        // TODO: The linkFramework string should be validated and escaped for security!!!
        StringAppendFormat(command, " -framework %s", StringGetCharacters(linkFramework));
    }

    // TODO: macOS min version is missing
    // TODO: This libSystem should only be linked on macOS
    // dynamic main executables must link with libSystem.dylib for inferred architecture x86_64
    // See: https://stackoverflow.com/questions/52830484/nasm-cant-link-object-file-with-ld-on-macos-mojave
    if (targetType == LDLinkerTargetTypeExecutable) {
        StringAppendFormat(command, " %s %s", "-L$(xcrun --sdk macosx --show-sdk-path)/usr/lib", "-lSystem");
    }
    
    Int status = system(StringGetCharacters(command));
    if (status != EXIT_SUCCESS) {
        ReportError("Linking process failed!");
    }

    StringDestroy(command);
}
