#include "JellyCore/ASTDumper.h"
#include "JellyCore/Allocator.h"
#include "JellyCore/Compiler.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/Workspace.h"

#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

Int CompilerRun(ArrayRef arguments) {
    Int32 argc  = ArrayGetElementCount(arguments);
    Char **argv = AllocatorAllocate(AllocatorGetSystemDefault(), sizeof(Char *) * argc);
    for (Index index = 0; index < argc; index++) {
        StringRef argument = *((StringRef *)ArrayGetElementAtIndex(arguments, index));
        argv[index]        = StringGetCharacters(argument);
    }

    Int32 optionDumpAST          = 0;
    Int32 optionWorkingDirectory = 0;
    StringRef dumpFilePath       = NULL;
    StringRef workingDirectory   = NULL;

    struct option options[] = {
        {"dump-ast", optional_argument, &optionDumpAST, 1},
        {"working-directory", required_argument, &optionWorkingDirectory, 1},
        {0, 0, 0, 0},
    };

    optind = 1;
    Bool parsed = false;
    while (!parsed) {
        int index = 0;

        switch (getopt_long_only(argc, argv, "", options, &index)) {
        case 0:
            if (index == 0 && optarg) {
                dumpFilePath = StringCreate(AllocatorGetSystemDefault(), optarg);
            }

            if (index == 1) {
                workingDirectory = StringCreate(AllocatorGetSystemDefault(), optarg);
            }
            break;

        case '?':
            break;

        case -1:
            parsed = true;
            break;

        default:
            if (dumpFilePath) {
                StringDestroy(dumpFilePath);
            }

            if (workingDirectory) {
                StringDestroy(workingDirectory);
            }

            return EXIT_FAILURE;
        }
    }

    if (!optionWorkingDirectory) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            workingDirectory = StringCreate(AllocatorGetSystemDefault(), cwd);
        } else {
            ReportCritical("Couldn't read current working directory");

            if (dumpFilePath) {
                StringDestroy(dumpFilePath);
            }

            if (workingDirectory) {
                StringDestroy(workingDirectory);
            }

            return EXIT_FAILURE;
        }
    }

    WorkspaceRef workspace = WorkspaceCreate(AllocatorGetSystemDefault(), workingDirectory);

    if (optind < argc) {
        StringRef filePath = StringCreate(AllocatorGetSystemDefault(), argv[optind]);
        WorkspaceAddSourceFile(workspace, filePath);
        StringDestroy(filePath);
        optind += 1;
    }

    WorkspaceStartAsync(workspace);
    WorkspaceWaitForFinish(workspace);

    if (optionDumpAST) {
        FILE *output = stdout;
        if (dumpFilePath) {
            output = fopen(StringGetCharacters(dumpFilePath), "w");
        }

        ASTContextRef context = WorkspaceGetContext(workspace);
        ASTDumperRef dumper   = ASTDumperCreate(AllocatorGetSystemDefault(), output);
        ASTDumperDump(dumper, (ASTNodeRef)ASTContextGetModule(context));
        ASTDumperDestroy(dumper);

        if (dumpFilePath) {
            fclose(output);
        }
    }

    WorkspaceDestroy(workspace);

    if (dumpFilePath) {
        StringDestroy(dumpFilePath);
    }

    StringDestroy(workingDirectory);
    return EXIT_SUCCESS;
}
