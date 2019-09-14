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

// TODO: @ModuleSupport Add compilation support of modules

Int CompilerRun(ArrayRef arguments) {
    Int32 argc  = ArrayGetElementCount(arguments);
    Char **argv = AllocatorAllocate(AllocatorGetSystemDefault(), sizeof(Char *) * argc);
    for (Index index = 0; index < argc; index++) {
        StringRef argument = *((StringRef *)ArrayGetElementAtIndex(arguments, index));
        argv[index]        = StringGetCharacters(argument);
    }

    Int32 optionDumpAST          = 0;
    Int32 optionDumpScope        = 0;
    Int32 optionDumpIR           = 0;
    Int32 optionWorkingDirectory = 0;
    Int32 optionModuleName       = 0;
    Int32 optionTypeCheck        = 0;
    StringRef dumpASTFilePath    = NULL;
    StringRef dumpScopeFilePath  = NULL;
    StringRef workingDirectory   = NULL;
    StringRef moduleName         = NULL;

    struct option options[] = {
        {"dump-ast", optional_argument, &optionDumpAST, 1},
        {"dump-scope", optional_argument, &optionDumpScope, 1},
        {"dump-ir", no_argument, &optionDumpIR, 1},
        {"working-directory", required_argument, &optionWorkingDirectory, 1},
        {"module-name", optional_argument, &optionModuleName, 1},
        {"type-check", no_argument, &optionTypeCheck, 1},
        {0, 0, 0, 0},
    };

    optind      = 1;
    Bool parsed = false;
    while (!parsed) {
        int index = 0;

        switch (getopt_long_only(argc, argv, "", options, &index)) {
        case 0:
            if (index == 0 && optarg) {
                dumpASTFilePath = StringCreate(AllocatorGetSystemDefault(), optarg);
            }

            if (index == 1 && optarg) {
                dumpScopeFilePath = StringCreate(AllocatorGetSystemDefault(), optarg);
            }

            if (index == 3) {
                workingDirectory = StringCreate(AllocatorGetSystemDefault(), optarg);
            }

            if (index == 4 && optarg) {
                moduleName = StringCreate(AllocatorGetSystemDefault(), optarg);
            }
            break;

        case '?':
            break;

        case -1:
            parsed = true;
            break;

        default:
            if (dumpASTFilePath) {
                StringDestroy(dumpASTFilePath);
            }

            if (dumpScopeFilePath) {
                StringDestroy(dumpScopeFilePath);
            }

            if (workingDirectory) {
                StringDestroy(workingDirectory);
            }

            if (moduleName) {
                StringDestroy(moduleName);
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

            if (dumpASTFilePath) {
                StringDestroy(dumpASTFilePath);
            }

            if (dumpScopeFilePath) {
                StringDestroy(dumpScopeFilePath);
            }

            if (workingDirectory) {
                StringDestroy(workingDirectory);
            }

            if (moduleName) {
                StringDestroy(moduleName);
            }

            return EXIT_FAILURE;
        }
    }

    WorkspaceOptions workspaceOptions = WorkspaceOptionsNone;
    if (optionDumpAST) {
        workspaceOptions |= WorkspaceOptionsDumpAST;
    }

    if (optionDumpScope) {
        workspaceOptions |= WorkspaceOptionsDumpScope;
    }

    if (optionDumpIR) {
        workspaceOptions |= WorkspaceOptionsDumpIR;
    }

    if (optionTypeCheck) {
        workspaceOptions |= WorkspaceOptionsTypeCheck;
    }

    StringRef buildDirectory = StringCreateCopy(AllocatorGetSystemDefault(), workingDirectory);
    StringAppend(buildDirectory, "/build");

    if (!moduleName) {
        moduleName = StringCreate(AllocatorGetSystemDefault(), "Module");
    }

    WorkspaceRef workspace = WorkspaceCreate(AllocatorGetSystemDefault(), workingDirectory, buildDirectory, moduleName, workspaceOptions);

    FILE *dumpASTOutput = NULL;
    if (dumpASTFilePath) {
        dumpASTOutput = fopen(StringGetCharacters(dumpASTFilePath), "w");
        assert(dumpASTOutput);
        WorkspaceSetDumpASTOutput(workspace, dumpASTOutput);
    }

    FILE *dumpScopeOutput = NULL;
    if (dumpScopeFilePath) {
        dumpScopeOutput = fopen(StringGetCharacters(dumpScopeFilePath), "w");
        assert(dumpScopeOutput);
        WorkspaceSetDumpScopeOutput(workspace, dumpScopeOutput);
    }

    if (optind < argc) {
        StringRef filePath = StringCreate(AllocatorGetSystemDefault(), argv[optind]);
        WorkspaceAddSourceFile(workspace, filePath);
        StringDestroy(filePath);
        optind += 1;
    }

    WorkspaceStartAsync(workspace);
    WorkspaceWaitForFinish(workspace);
    WorkspaceDestroy(workspace);

    if (dumpASTOutput) {
        fclose(dumpASTOutput);
    }

    if (dumpASTFilePath) {
        StringDestroy(dumpASTFilePath);
    }

    if (dumpScopeOutput) {
        fclose(dumpScopeOutput);
    }

    if (dumpScopeFilePath) {
        StringDestroy(dumpScopeFilePath);
    }

    StringDestroy(moduleName);
    StringDestroy(buildDirectory);
    StringDestroy(workingDirectory);
    return EXIT_SUCCESS;
}
