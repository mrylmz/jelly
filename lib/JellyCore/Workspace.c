#include "JellyCore/ASTDumper.h"
#include "JellyCore/ASTMangling.h"
#include "JellyCore/ASTScope.h"
#include "JellyCore/ASTSubstitution.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/IRBuilder.h"
#include "JellyCore/LDLinker.h"
#include "JellyCore/NameResolution.h"
#include "JellyCore/Parser.h"
#include "JellyCore/Queue.h"
#include "JellyCore/TypeChecker.h"
#include "JellyCore/Workspace.h"

#include <pthread.h>

// TODO: Fix memory leaks ...

struct _Workspace {
    AllocatorRef allocator;
    StringRef workingDirectory;
    StringRef buildDirectory;
    ArrayRef sourceFilePaths;
    ArrayRef moduleFilePaths;
    ASTContextRef context;
    ParserRef parser;
    QueueRef parseQueue;
    QueueRef parseInterfaceQueue;
    QueueRef importQueue;

    WorkspaceOptions options;
    FILE *dumpASTOutput;
    FILE *dumpScopeOutput;

    Bool running;
    Bool waiting;
    pthread_mutex_t mutex;
    pthread_mutex_t empty;
    pthread_t thread;
};

Bool _ArrayContainsString(const void *lhs, const void *rhs);

void _WorkspacePerformLoads(WorkspaceRef workspace, ASTSourceUnitRef sourceUnit);
void _WorkspacePerformInterfaceLoads(WorkspaceRef workspace, ASTModuleDeclarationRef module, ASTSourceUnitRef sourceUnit);
void _WorkspacePerformImports(WorkspaceRef workspace, ASTModuleDeclarationRef module, ASTSourceUnitRef sourceUnit);
void _WorkspaceParse(void *argument, void *context);
void *_WorkspaceProcess(void *context);

WorkspaceRef WorkspaceCreate(AllocatorRef allocator, StringRef workingDirectory, StringRef buildDirectory, StringRef moduleName,
                             WorkspaceOptions options) {
    WorkspaceRef workspace         = AllocatorAllocate(allocator, sizeof(struct _Workspace));
    workspace->allocator           = allocator;
    workspace->workingDirectory    = StringCreateCopy(allocator, workingDirectory);
    workspace->buildDirectory      = StringCreateCopy(allocator, buildDirectory);
    workspace->sourceFilePaths     = ArrayCreateEmpty(allocator, sizeof(StringRef *), 8);
    workspace->moduleFilePaths     = ArrayCreateEmpty(allocator, sizeof(StringRef *), 8);
    workspace->context             = ASTContextCreate(allocator, moduleName);
    workspace->parser              = ParserCreate(allocator, workspace->context);
    workspace->parseQueue          = QueueCreate(allocator);
    workspace->parseInterfaceQueue = QueueCreate(allocator);
    workspace->importQueue         = QueueCreate(allocator);
    workspace->options             = options;
    workspace->dumpASTOutput       = stdout;
    workspace->dumpScopeOutput     = stdout;
    workspace->running             = false;
    workspace->waiting             = false;
    return workspace;
}

void WorkspaceDestroy(WorkspaceRef workspace) {
    if (workspace->running && !workspace->waiting) {
        WorkspaceWaitForFinish(workspace);
    }

    ArrayDestroy(workspace->sourceFilePaths);
    QueueDestroy(workspace->importQueue);
    QueueDestroy(workspace->parseInterfaceQueue);
    QueueDestroy(workspace->parseQueue);
    ParserDestroy(workspace->parser);
    ASTContextDestroy(workspace->context);
    StringDestroy(workspace->buildDirectory);
    StringDestroy(workspace->workingDirectory);
    AllocatorDeallocate(workspace->allocator, workspace);
}

ASTContextRef WorkspaceGetContext(WorkspaceRef workspace) {
    return workspace->context;
}

void WorkspaceAddSourceFile(WorkspaceRef workspace, StringRef filePath) {
    StringRef absoluteFilePath = StringCreateCopy(workspace->allocator, workspace->workingDirectory);
    StringAppend(absoluteFilePath, "/");
    StringAppendString(absoluteFilePath, filePath);
    if (ArrayContainsElement(workspace->sourceFilePaths, &_ArrayContainsString, &absoluteFilePath)) {
        ReportErrorFormat("Cannot load source file at path '%s' twice", StringGetCharacters(filePath));
        StringDestroy(absoluteFilePath);
        return;
    }

    ArrayAppendElement(workspace->sourceFilePaths, &absoluteFilePath);

    StringRef copy = StringCreateCopy(workspace->allocator, filePath);
    pthread_mutex_lock(&workspace->mutex);
    QueueEnqueue(workspace->parseQueue, copy);
    pthread_mutex_unlock(&workspace->mutex);
}

void WorkspaceSetDumpASTOutput(WorkspaceRef workspace, FILE *output) {
    assert(output);
    workspace->dumpASTOutput = output;
}

void WorkspaceSetDumpScopeOutput(WorkspaceRef workspace, FILE *output) {
    assert(output);
    workspace->dumpScopeOutput = output;
}

Bool WorkspaceStartAsync(WorkspaceRef workspace) {
    assert(!workspace->running);
    workspace->running = true;

    DiagnosticEngineResetMessageCounts();
    pthread_mutex_init(&workspace->mutex, NULL);
    pthread_mutex_init(&workspace->empty, NULL);
    return pthread_create(&workspace->thread, NULL, &_WorkspaceProcess, workspace) == 0;
}

void WorkspaceWaitForFinish(WorkspaceRef workspace) {
    assert(workspace->running);
    assert(!workspace->waiting);
    workspace->waiting = true;
    pthread_join(workspace->thread, NULL);
    workspace->running = false;
    workspace->waiting = false;
}

Bool _ArrayContainsString(const void *lhs, const void *rhs) {
    return StringIsEqual(*((StringRef *)lhs), *((StringRef *)rhs));
}

void _WorkspacePerformLoads(WorkspaceRef workspace, ASTSourceUnitRef sourceUnit) {
    for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
        ASTNodeRef node = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
        if (node->tag == ASTTagLoadDirective) {
            ASTLoadDirectiveRef load = (ASTLoadDirectiveRef)node;
            assert(load->filePath->kind == ASTConstantKindString);
            StringRef filePath         = load->filePath->stringValue;
            StringRef relativeFilePath = StringCreateCopyUntilLastOccurenceOf(workspace->allocator, sourceUnit->filePath, '/');
            if (StringGetLength(relativeFilePath) > 0) {
                StringAppend(relativeFilePath, "/");
            }
            StringAppendString(relativeFilePath, filePath);

            StringRef absoluteFilePath = StringCreateCopy(workspace->allocator, workspace->workingDirectory);
            if (StringGetLength(absoluteFilePath) > 0) {
                StringAppend(absoluteFilePath, "/");
            }
            StringAppendString(absoluteFilePath, relativeFilePath);

            if (ArrayContainsElement(workspace->sourceFilePaths, &_ArrayContainsString, &absoluteFilePath)) {
                ReportErrorFormat("Cannot load source file at path '%s' twice", StringGetCharacters(filePath));
                StringDestroy(relativeFilePath);
                StringDestroy(absoluteFilePath);
            } else {
                ArrayAppendElement(workspace->sourceFilePaths, &absoluteFilePath);
                pthread_mutex_lock(&workspace->mutex);
                QueueEnqueue(workspace->parseQueue, relativeFilePath);
                pthread_mutex_unlock(&workspace->mutex);
            }
        }
    }
}

void _WorkspacePerformInterfaceLoads(WorkspaceRef workspace, ASTModuleDeclarationRef module, ASTSourceUnitRef sourceUnit) {
    ASTArrayIteratorRef iterator = ASTArrayGetIterator(sourceUnit->declarations);
    while (iterator) {
        ASTNodeRef node = (ASTNodeRef)ASTArrayIteratorGetElement(iterator);
        if (node->tag == ASTTagLoadDirective) {
            ASTLoadDirectiveRef load = (ASTLoadDirectiveRef)node;
            assert(load->filePath->kind == ASTConstantKindString);
            StringRef filePath         = load->filePath->stringValue;
            StringRef relativeFilePath = StringCreateCopyUntilLastOccurenceOf(workspace->allocator, sourceUnit->filePath, '/');
            if (StringGetLength(relativeFilePath) > 0) {
                StringAppend(relativeFilePath, "/");
            }
            StringAppendString(relativeFilePath, filePath);

            StringRef absoluteFilePath = StringCreateCopy(workspace->allocator, workspace->workingDirectory);
            if (StringGetLength(absoluteFilePath) > 0) {
                StringAppend(absoluteFilePath, "/");
            }
            StringAppendString(absoluteFilePath, relativeFilePath);

            if (ArrayContainsElement(workspace->sourceFilePaths, &_ArrayContainsString, &absoluteFilePath)) {
                ReportErrorFormat("Cannot load source file at path '%s' twice", StringGetCharacters(filePath));
                StringDestroy(relativeFilePath);
                StringDestroy(absoluteFilePath);
            } else {
                ArrayAppendElement(workspace->sourceFilePaths, &absoluteFilePath);
                pthread_mutex_lock(&workspace->mutex);
                QueueEnqueue(workspace->parseInterfaceQueue, module);
                QueueEnqueue(workspace->parseInterfaceQueue, relativeFilePath);
                pthread_mutex_unlock(&workspace->mutex);
            }
        }

        iterator = ASTArrayIteratorNext(iterator);
    }
}

void _WorkspacePerformImports(WorkspaceRef workspace, ASTModuleDeclarationRef module, ASTSourceUnitRef sourceUnit) {
    ASTArrayIteratorRef iterator = ASTArrayGetIterator(sourceUnit->declarations);
    while (iterator) {
        ASTNodeRef node = (ASTNodeRef)ASTArrayIteratorGetElement(iterator);
        if (node->tag == ASTTagImportDirective) {
            ASTImportDirectiveRef import = (ASTImportDirectiveRef)node;
            StringRef relativeFilePath   = StringCreateCopyUntilLastOccurenceOf(workspace->allocator, sourceUnit->filePath, '/');
            if (StringGetLength(relativeFilePath) > 0) {
                StringAppend(relativeFilePath, "/");
            }
            StringAppendString(relativeFilePath, import->modulePath);

            StringRef absoluteFilePath = StringCreateCopy(workspace->allocator, workspace->workingDirectory);
            if (StringGetLength(absoluteFilePath) > 0) {
                StringAppend(absoluteFilePath, "/");
            }
            StringAppendString(absoluteFilePath, relativeFilePath);

            if (ArrayContainsElement(workspace->moduleFilePaths, &_ArrayContainsString, &absoluteFilePath)) {
                ReportErrorFormat("Cannot import module file at path '%s' twice", StringGetCharacters(import->modulePath));
                StringDestroy(relativeFilePath);
                StringDestroy(absoluteFilePath);
            } else {
                ArrayAppendElement(workspace->moduleFilePaths, &absoluteFilePath);
                pthread_mutex_lock(&workspace->mutex);
                QueueEnqueue(workspace->importQueue, module);
                QueueEnqueue(workspace->importQueue, relativeFilePath);
                pthread_mutex_unlock(&workspace->mutex);
            }
        }

        iterator = ASTArrayIteratorNext(iterator);
    }
}

void *_WorkspaceProcess(void *context) {
    WorkspaceRef workspace = (WorkspaceRef)context;

    // Parse / Import phase
    while (true) {
        pthread_mutex_lock(&workspace->mutex);
        StringRef parseFilePath = QueueDequeue(workspace->parseQueue);
        pthread_mutex_unlock(&workspace->mutex);

        if (parseFilePath) {
            StringRef absoluteFilePath = StringCreateCopy(workspace->allocator, workspace->workingDirectory);
            StringAppend(absoluteFilePath, "/");
            StringAppendString(absoluteFilePath, parseFilePath);
            StringRef source = StringCreateFromFile(workspace->allocator, StringGetCharacters(absoluteFilePath));
            if (source) {
                ASTSourceUnitRef sourceUnit = ParserParseSourceUnit(workspace->parser, parseFilePath, source);
                StringDestroy(source);
                _WorkspacePerformLoads(workspace, sourceUnit);
                _WorkspacePerformImports(workspace, ASTContextGetModule(workspace->context), sourceUnit);
            } else {
                ReportErrorFormat("File not found: '%s'", StringGetCharacters(parseFilePath));
            }

            StringDestroy(absoluteFilePath);
            StringDestroy(parseFilePath);
        }

        pthread_mutex_lock(&workspace->mutex);
        ASTModuleDeclarationRef module = QueueDequeue(workspace->importQueue);
        StringRef importFilePath       = QueueDequeue(workspace->importQueue);
        pthread_mutex_unlock(&workspace->mutex);

        if (importFilePath) {
            StringRef absoluteFilePath = StringCreateCopy(workspace->allocator, workspace->workingDirectory);
            StringAppend(absoluteFilePath, "/");
            StringAppendString(absoluteFilePath, importFilePath);
            StringRef source = StringCreateFromFile(workspace->allocator, StringGetCharacters(absoluteFilePath));
            if (source) {
                ArrayRef modules  = ASTContextGetAllNodes(workspace->context, ASTTagModuleDeclaration);
                Index moduleCount = ArrayGetElementCount(modules);

                ASTModuleDeclarationRef importedModule = ParserParseModuleDeclaration(workspace->parser, importFilePath, source);
                StringDestroy(source);

                if (importedModule) {
                    assert(ASTArrayGetElementCount(importedModule->sourceUnits) == 1);
                    ASTSourceUnitRef sourceUnit = ASTArrayGetElementAtIndex(importedModule->sourceUnits, 0);
                    _WorkspacePerformInterfaceLoads(workspace, importedModule, sourceUnit);

                    ASTArrayAppendElement(module->importedModules, importedModule);

                    for (Index index = 0; index < moduleCount; index++) {
                        ASTModuleDeclarationRef loadedModule = (ASTModuleDeclarationRef)ArrayGetElementAtIndex(modules, index);
                        if (StringIsEqual(importedModule->base.name, loadedModule->base.name)) {
                            ReportErrorFormat("Module '%s' cannot be imported twice", StringGetCharacters(importedModule->base.name));
                        }
                    }
                }
            } else {
                ReportErrorFormat("File not found: '%s'", StringGetCharacters(importFilePath));
            }

            StringDestroy(absoluteFilePath);
            StringDestroy(importFilePath);
        }

        pthread_mutex_lock(&workspace->mutex);
        ASTModuleDeclarationRef importedModule = QueueDequeue(workspace->parseInterfaceQueue);
        StringRef parseInterfaceFilePath       = QueueDequeue(workspace->parseInterfaceQueue);
        pthread_mutex_unlock(&workspace->mutex);

        if (importedModule && parseInterfaceFilePath) {
            StringRef absoluteFilePath = StringCreateCopy(workspace->allocator, workspace->workingDirectory);
            StringAppend(absoluteFilePath, "/");
            StringAppendString(absoluteFilePath, parseInterfaceFilePath);
            StringRef source = StringCreateFromFile(workspace->allocator, StringGetCharacters(absoluteFilePath));
            if (source) {
                ASTSourceUnitRef sourceUnit = ParserParseModuleSourceUnit(workspace->parser, importedModule, parseInterfaceFilePath,
                                                                          source);
                StringDestroy(source);
                _WorkspacePerformInterfaceLoads(workspace, importedModule, sourceUnit);
            } else {
                ReportErrorFormat("File not found: '%s'", StringGetCharacters(parseInterfaceFilePath));
            }

            StringDestroy(absoluteFilePath);
            StringDestroy(parseInterfaceFilePath);
        }

        if (!parseFilePath && !importFilePath && !parseInterfaceFilePath) {
            break;
        }
    }

    if ((workspace->options & WorkspaceOptionsDumpAST) > 0) {
        ASTDumperRef dumper = ASTDumperCreate(workspace->allocator, workspace->dumpASTOutput);
        ASTDumperDump(dumper, (ASTNodeRef)ASTContextGetModule(workspace->context));
        ASTDumperDestroy(dumper);
        // TODO: Verify if early return is correct behaviour here...
        //       currently we are exiting at this point because we have parsed the full AST here,
        //       but it could be that code can be generated soon
        return NULL;
    }

    if (DiagnosticEngineGetMessageCount(DiagnosticLevelError) > 0 || DiagnosticEngineGetMessageCount(DiagnosticLevelCritical) > 0) {
        return NULL;
    }

    ASTPerformSubstitution(workspace->context, ASTTagUnaryExpression, &ASTUnaryExpressionUnification);
    ASTPerformSubstitution(workspace->context, ASTTagBinaryExpression, &ASTBinaryExpressionUnification);
    ASTApplySubstitution(workspace->context, ASTContextGetModule(workspace->context));

    PerformNameResolution(workspace->context, ASTContextGetModule(workspace->context));

    // Perform ASTApplySubstitution a second time to allow substitutions in name resolution phase...
    ASTApplySubstitution(workspace->context, ASTContextGetModule(workspace->context));

    if ((workspace->options & WorkspaceOptionsDumpScope) > 0) {
        ASTScopeDump(ASTContextGetGlobalScope(workspace->context), workspace->dumpScopeOutput);
    }

    TypeCheckerRef typeChecker = TypeCheckerCreate(workspace->allocator);
    TypeCheckerValidateModule(typeChecker, workspace->context, ASTContextGetModule(workspace->context));
    TypeCheckerDestroy(typeChecker);

    if (DiagnosticEngineGetMessageCount(DiagnosticLevelError) > 0 || DiagnosticEngineGetMessageCount(DiagnosticLevelCritical) > 0) {
        return NULL;
    }

    if (workspace->options & WorkspaceOptionsTypeCheck) {
        return NULL;
    }

    PerformNameMangling(workspace->context, ASTContextGetModule(workspace->context));

    DIR *buildDirectory = opendir(StringGetCharacters(workspace->buildDirectory));
    if (!buildDirectory) {
        if (mkdir(StringGetCharacters(workspace->buildDirectory), S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
            ReportErrorFormat("Couldn't create build directory at path: '%s'", StringGetCharacters(workspace->buildDirectory));
        }
    } else {
        closedir(buildDirectory);
    }

    if (DiagnosticEngineGetMessageCount(DiagnosticLevelError) > 0 || DiagnosticEngineGetMessageCount(DiagnosticLevelCritical) > 0) {
        return NULL;
    }

    IRBuilderRef builder = IRBuilderCreate(workspace->allocator, workspace->context, workspace->buildDirectory);
    IRModuleRef module   = IRBuilderBuild(builder, ASTContextGetModule(workspace->context));

    if ((workspace->options & WorkspaceOptionsDumpIR) > 0) {
        IRBuilderDumpModule(builder, module, stdout);
        IRBuilderDestroy(builder);
        return NULL;
    }

    IRBuilderVerifyModule(builder, module);

    if (DiagnosticEngineGetMessageCount(DiagnosticLevelError) > 0 || DiagnosticEngineGetMessageCount(DiagnosticLevelCritical) > 0) {
        IRBuilderDestroy(builder);
        return NULL;
    }

    IRBuilderEmitObjectFile(builder, module, ASTContextGetModule(workspace->context)->base.name);
    IRBuilderDestroy(builder);

    if (DiagnosticEngineGetMessageCount(DiagnosticLevelError) > 0 || DiagnosticEngineGetMessageCount(DiagnosticLevelCritical) > 0) {
        return NULL;
    }

    ArrayRef objectFiles     = ArrayCreateEmpty(workspace->allocator, sizeof(StringRef), 1);
    StringRef objectFilePath = StringCreateCopy(workspace->allocator, workspace->buildDirectory);
    StringAppendFormat(objectFilePath, "/%s.o", StringGetCharacters(ASTContextGetModule(workspace->context)->base.name));
    ArrayAppendElement(objectFiles, &objectFilePath);

    StringRef targetPath = StringCreateCopy(workspace->allocator, workspace->buildDirectory);
    StringAppendFormat(targetPath, "/program");

    ArrayRef linkLibraries       = ArrayCreateEmpty(workspace->allocator, sizeof(StringRef), 0);
    ArrayRef linkFrameworks      = ArrayCreateEmpty(workspace->allocator, sizeof(StringRef), 0);
    ASTArrayIteratorRef iterator = ASTArrayGetIterator(ASTContextGetModule(workspace->context)->linkDirectives);
    while (iterator) {
        ASTLinkDirectiveRef link = (ASTLinkDirectiveRef)ASTArrayIteratorGetElement(iterator);
        if (link->isFramework) {
            ArrayAppendElement(linkFrameworks, &link->library);
        } else {
            ArrayAppendElement(linkLibraries, &link->library);
        }

        iterator = ASTArrayIteratorNext(iterator);
    }

    LDLinkerLink(workspace->allocator, objectFiles, linkLibraries, linkFrameworks, targetPath, LDLinkerTargetTypeExecutable, NULL);

    StringDestroy(targetPath);
    ArrayDestroy(objectFiles);
    StringDestroy(objectFilePath);

    return NULL;
}
