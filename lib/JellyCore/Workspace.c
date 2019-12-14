#include "JellyCore/ASTDumper.h"
#include "JellyCore/ASTMangling.h"
#include "JellyCore/ASTSubstitution.h"
#include "JellyCore/ClangImporter.h"
#include "JellyCore/DependencyGraph.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/Dictionary.h"
#include "JellyCore/IRBuilder.h"
#include "JellyCore/LDLinker.h"
#include "JellyCore/NameResolution.h"
#include "JellyCore/Parser.h"
#include "JellyCore/Queue.h"
#include "JellyCore/TypeChecker.h"
#include "JellyCore/Workspace.h"

#include <pthread.h>

struct _Workspace {
    AllocatorRef allocator;
    StringRef workingDirectory;
    StringRef buildDirectory;
    ArrayRef sourceFilePaths;
    ArrayRef includeFilePaths;
    ArrayRef moduleFilePaths;
    ASTContextRef context;
    ParserRef parser;
    ClangImporterRef importer;
    QueueRef parseQueue;
    QueueRef parseInterfaceQueue;
    QueueRef parseIncludeQueue;
    QueueRef importQueue;
    DictionaryRef modules;

    WorkspaceOptions options;
    FILE *dumpASTOutput;

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
    workspace->includeFilePaths    = ArrayCreateEmpty(allocator, sizeof(StringRef *), 8);
    workspace->moduleFilePaths     = ArrayCreateEmpty(allocator, sizeof(StringRef *), 8);
    workspace->context             = ASTContextCreate(allocator, moduleName);
    workspace->parser              = ParserCreate(allocator, workspace->context);
    workspace->importer            = ClangImporterCreate(allocator, workspace->context);
    workspace->parseQueue          = QueueCreate(allocator);
    workspace->parseInterfaceQueue = QueueCreate(allocator);
    workspace->parseIncludeQueue   = QueueCreate(allocator);
    workspace->importQueue         = QueueCreate(allocator);
    workspace->modules             = CStringDictionaryCreate(allocator, 8);
    workspace->options             = options;
    workspace->dumpASTOutput       = stdout;
    workspace->running             = false;
    workspace->waiting             = false;
    pthread_mutex_init(&workspace->mutex, NULL);
    pthread_mutex_init(&workspace->empty, NULL);

    ASTModuleDeclarationRef module = ASTContextGetModule(workspace->context);
    DictionaryInsert(workspace->modules, StringGetCharacters(module->base.name), &module, sizeof(ASTModuleDeclarationRef));
    return workspace;
}

void WorkspaceDestroy(WorkspaceRef workspace) {
    if (workspace->running && !workspace->waiting) {
        WorkspaceWaitForFinish(workspace);
    }

    for (Index index = 0; index < ArrayGetElementCount(workspace->moduleFilePaths); index++) {
        StringRef string = *(StringRef *)ArrayGetElementAtIndex(workspace->moduleFilePaths, index);
        StringDestroy(string);
    }

    for (Index index = 0; index < ArrayGetElementCount(workspace->includeFilePaths); index++) {
        StringRef string = *(StringRef *)ArrayGetElementAtIndex(workspace->includeFilePaths, index);
        StringDestroy(string);
    }

    for (Index index = 0; index < ArrayGetElementCount(workspace->sourceFilePaths); index++) {
        StringRef string = *(StringRef *)ArrayGetElementAtIndex(workspace->sourceFilePaths, index);
        StringDestroy(string);
    }

    StringDestroy(workspace->workingDirectory);
    StringDestroy(workspace->buildDirectory);
    ArrayDestroy(workspace->sourceFilePaths);
    ArrayDestroy(workspace->includeFilePaths);
    ArrayDestroy(workspace->moduleFilePaths);
    ASTContextDestroy(workspace->context);
    ParserDestroy(workspace->parser);
    ClangImporterDestroy(workspace->importer);
    QueueDestroy(workspace->parseQueue);
    QueueDestroy(workspace->parseInterfaceQueue);
    QueueDestroy(workspace->parseIncludeQueue);
    QueueDestroy(workspace->importQueue);
    DictionaryDestroy(workspace->modules);
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

Bool WorkspaceStartAsync(WorkspaceRef workspace) {
    assert(!workspace->running);
    workspace->running = true;

    DiagnosticEngineResetMessageCounts();
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

void _WorkspacePerformIncludes(WorkspaceRef workspace, ASTModuleDeclarationRef module, ASTSourceUnitRef sourceUnit) {
    ASTArrayIteratorRef iterator = ASTArrayGetIterator(sourceUnit->declarations);
    while (iterator) {
        ASTNodeRef node = (ASTNodeRef)ASTArrayIteratorGetElement(iterator);
        if (node->tag == ASTTagIncludeDirective) {
            ASTIncludeDirectiveRef include = (ASTIncludeDirectiveRef)node;

            // TODO: Add header search paths for lookup!
            StringRef relativeFilePath = StringCreateCopyUntilLastOccurenceOf(workspace->allocator, sourceUnit->filePath, '/');
            if (StringGetLength(relativeFilePath) > 0) {
                StringAppend(relativeFilePath, "/");
            }
            StringAppendString(relativeFilePath, include->headerPath);

            StringRef absoluteFilePath = StringCreateCopy(workspace->allocator, workspace->workingDirectory);
            if (StringGetLength(absoluteFilePath) > 0) {
                StringAppend(absoluteFilePath, "/");
            }
            StringAppendString(absoluteFilePath, relativeFilePath);

            if (ArrayContainsElement(workspace->includeFilePaths, &_ArrayContainsString, &absoluteFilePath)) {
                ReportErrorFormat("Cannot include header file at path '%s' twice", StringGetCharacters(include->headerPath));
                StringDestroy(relativeFilePath);
                StringDestroy(absoluteFilePath);
            } else {
                ArrayAppendElement(workspace->includeFilePaths, &absoluteFilePath);
                pthread_mutex_lock(&workspace->mutex);
                QueueEnqueue(workspace->parseIncludeQueue, module);
                QueueEnqueue(workspace->parseIncludeQueue, relativeFilePath);
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

Bool _WorkspaceProcessParseQueue(WorkspaceRef workspace) {
    Index processedFileCount = 0;

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
                _WorkspacePerformIncludes(workspace, ASTContextGetModule(workspace->context), sourceUnit);
            } else {
                ReportErrorFormat("File not found: '%s'", StringGetCharacters(parseFilePath));
            }

            StringDestroy(absoluteFilePath);
            StringDestroy(parseFilePath);

            processedFileCount += 1;
        } else {
            break;
        }
    }

    return processedFileCount > 0;
}

Bool _WorkspaceProcessImportQueue(WorkspaceRef workspace) {
    Index processedFileCount = 0;

    while (true) {
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
                ASTModuleDeclarationRef importedModule = ParserParseModuleDeclaration(workspace->parser, importFilePath, source);
                StringDestroy(source);

                if (importedModule) {
                    if (DictionaryLookup(workspace->modules, StringGetCharacters(importedModule->base.name)) != NULL) {
                        ReportErrorFormat("Module '%s' cannot be imported twice", StringGetCharacters(importedModule->base.name));
                    }

                    assert(ASTArrayGetElementCount(importedModule->sourceUnits) == 1);
                    ASTSourceUnitRef sourceUnit = ASTArrayGetElementAtIndex(importedModule->sourceUnits, 0);
                    _WorkspacePerformInterfaceLoads(workspace, importedModule, sourceUnit);
                    ASTArrayAppendElement(module->importedModules, importedModule);
                    DictionaryInsert(workspace->modules, StringGetCharacters(importedModule->base.name), &importedModule,
                                     sizeof(ASTModuleDeclarationRef));
                }
            } else {
                ReportErrorFormat("File not found: '%s'", StringGetCharacters(importFilePath));
            }

            StringDestroy(absoluteFilePath);
            StringDestroy(importFilePath);

            processedFileCount += 1;
        } else {
            break;
        }
    }

    return processedFileCount > 0;
}

Bool _WorkspaceProcessParseInterfaceQueue(WorkspaceRef workspace) {
    Index processedFileCount = 0;

    while (true) {
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

            processedFileCount += 1;
        } else {
            break;
        }
    }

    return processedFileCount > 0;
}

Bool _WorkspaceProcessParseIncludeQueue(WorkspaceRef workspace) {
    Index processedFileCount = 0;

    while (true) {
        pthread_mutex_lock(&workspace->mutex);
        ASTModuleDeclarationRef module = QueueDequeue(workspace->parseIncludeQueue);
        StringRef parseIncludeFilePath = QueueDequeue(workspace->parseIncludeQueue);
        pthread_mutex_unlock(&workspace->mutex);

        if (module && parseIncludeFilePath) {
            StringRef absoluteFilePath = StringCreateCopy(workspace->allocator, workspace->workingDirectory);
            StringAppend(absoluteFilePath, "/");
            StringAppendString(absoluteFilePath, parseIncludeFilePath);

            ASTModuleDeclarationRef importedModule = ClangImporterImport(workspace->importer, absoluteFilePath);
            if (importedModule) {
                if (DictionaryLookup(workspace->modules, StringGetCharacters(importedModule->base.name)) != NULL) {
                    ReportErrorFormat("Module '%s' cannot be imported twice", StringGetCharacters(importedModule->base.name));
                }

                ASTArrayAppendElement(module->importedModules, importedModule);
                DictionaryInsert(workspace->modules, StringGetCharacters(importedModule->base.name), &importedModule,
                                 sizeof(ASTModuleDeclarationRef));
            }

            StringDestroy(absoluteFilePath);
            StringDestroy(parseIncludeFilePath);

            processedFileCount += 1;
        } else {
            break;
        }
    }

    return processedFileCount > 0;
}

void _WorkspaceVerifyModule(WorkspaceRef workspace, ASTModuleDeclarationRef module) {
    ASTApplySubstitution(workspace->context, module);

    PerformNameResolution(workspace->context, module);

    // Perform ASTApplySubstitution a second time to allow substitutions in name resolution phase...
    ASTApplySubstitution(workspace->context, module);

    TypeCheckerRef typeChecker = TypeCheckerCreate(workspace->allocator);
    TypeCheckerValidateModule(typeChecker, workspace->context, module);
    TypeCheckerDestroy(typeChecker);
}

void _WorkspaceBuildModule(WorkspaceRef workspace, ASTModuleDeclarationRef module) {
    PerformNameMangling(workspace->context, module);

    if (module->kind == ASTModuleKindInterface) {
        return;
    }

    IRBuilderRef builder = IRBuilderCreate(workspace->allocator, workspace->context, workspace->buildDirectory);
    IRModuleRef irModule = IRBuilderBuild(builder, module);

    if ((workspace->options & WorkspaceOptionsDumpIR) > 0) {
        IRBuilderDumpModule(builder, irModule, stdout);
        IRBuilderDestroy(builder);
        return;
    }

    IRBuilderVerifyModule(builder, irModule);

    if (DiagnosticEngineGetMessageCount(DiagnosticLevelError) > 0 || DiagnosticEngineGetMessageCount(DiagnosticLevelCritical) > 0) {
        IRBuilderDestroy(builder);
        return;
    }

    IRBuilderEmitObjectFile(builder, irModule, module->base.name);
    IRBuilderDestroy(builder);
}

DependencyGraphNodeRef _DependencyGraphInsertModule(DependencyGraphRef graph, ASTModuleDeclarationRef module) {
    if (StringGetLength(module->base.name) < 1) {
        return NULL;
    }

    if (DependencyGraphLookupNode(graph, module->base.name)) {
        return NULL;
    }

    DependencyGraphNodeRef node  = DependencyGraphInsertNode(graph, module->base.name);
    ASTArrayIteratorRef iterator = ASTArrayGetIterator(module->importedModules);
    while (iterator) {
        ASTModuleDeclarationRef importedModule = (ASTModuleDeclarationRef)ASTArrayIteratorGetElement(iterator);
        DependencyGraphNodeRef child           = _DependencyGraphInsertModule(graph, importedModule);
        if (!child) {
            break;
        }

        Bool inserted = DependencyGraphAddDependency(graph, node, importedModule->base.name);
        assert(inserted);

        iterator = ASTArrayIteratorNext(iterator);
    }

    return node;
}

void *_WorkspaceProcess(void *context) {
    WorkspaceRef workspace = (WorkspaceRef)context;

    // Parse / Import phase
    Bool processFrontend = true;
    while (processFrontend) {
        processFrontend = false;
        processFrontend |= _WorkspaceProcessParseQueue(workspace);
        processFrontend |= _WorkspaceProcessImportQueue(workspace);
        processFrontend |= _WorkspaceProcessParseInterfaceQueue(workspace);
        processFrontend |= _WorkspaceProcessParseIncludeQueue(workspace);
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

    ASTModuleDeclarationRef astModule = ASTContextGetModule(workspace->context);
    DependencyGraphRef graph          = DependencyGraphCreate(workspace->allocator);
    _DependencyGraphInsertModule(graph, astModule);
    Bool hasCyclicDependencies;
    ArrayRef sortedModuleNames = DependencyGraphGetIdentifiersInTopologicalOrder(graph, &hasCyclicDependencies);
    if (hasCyclicDependencies) {
        ReportError("Found cyclic dependencies in imported modules!");
        ArrayDestroy(sortedModuleNames);
        return NULL;
    }

    ArrayRef sortedModules = ArrayCreateEmpty(workspace->allocator, sizeof(ASTModuleDeclarationRef),
                                              ArrayGetElementCount(sortedModuleNames));
    for (Index index = ArrayGetElementCount(sortedModuleNames); index > 0; index--) {
        StringRef moduleName           = *((StringRef *)ArrayGetElementAtIndex(sortedModuleNames, index - 1));
        ASTModuleDeclarationRef module = *(
            (ASTModuleDeclarationRef *)DictionaryLookup(workspace->modules, StringGetCharacters(moduleName)));
        if (module) {
            ArrayAppendElement(sortedModules, &module);
        } else {
            ReportErrorFormat("Couldn't find imported module '%s'", StringGetCharacters(moduleName));
        }
    }

    ArrayDestroy(sortedModuleNames);
    DependencyGraphDestroy(graph);

    ASTPerformSubstitution(workspace->context, ASTTagUnaryExpression, &ASTUnaryExpressionUnification);
    ASTPerformSubstitution(workspace->context, ASTTagBinaryExpression, &ASTBinaryExpressionUnification);

    for (Index index = 0; index < ArrayGetElementCount(sortedModules); index++) {
        ASTModuleDeclarationRef module = *((ASTModuleDeclarationRef *)ArrayGetElementAtIndex(sortedModules, index));
        _WorkspaceVerifyModule(workspace, module);
    }

    if (DiagnosticEngineGetMessageCount(DiagnosticLevelError) > 0 || DiagnosticEngineGetMessageCount(DiagnosticLevelCritical) > 0) {
        ArrayDestroy(sortedModules);
        return NULL;
    }

    if (workspace->options & WorkspaceOptionsTypeCheck) {
        ArrayDestroy(sortedModules);
        return NULL;
    }

    DIR *buildDirectory = opendir(StringGetCharacters(workspace->buildDirectory));
    if (!buildDirectory) {
        if (mkdir(StringGetCharacters(workspace->buildDirectory), S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
            ReportErrorFormat("Couldn't create build directory at path: '%s'", StringGetCharacters(workspace->buildDirectory));
        }
    } else {
        closedir(buildDirectory);
    }

    for (Index index = 0; index < ArrayGetElementCount(sortedModules); index++) {
        ASTModuleDeclarationRef module = *((ASTModuleDeclarationRef *)ArrayGetElementAtIndex(sortedModules, index));
        _WorkspaceBuildModule(workspace, module);
    }

    if (DiagnosticEngineGetMessageCount(DiagnosticLevelError) > 0 || DiagnosticEngineGetMessageCount(DiagnosticLevelCritical) > 0) {
        ArrayDestroy(sortedModules);
        return NULL;
    }

    if ((workspace->options & WorkspaceOptionsDumpIR) > 0) {
        ArrayDestroy(sortedModules);
        return NULL;
    }

    ArrayRef objectFiles = ArrayCreateEmpty(workspace->allocator, sizeof(StringRef), 1);
    for (Index index = 0; index < ArrayGetElementCount(sortedModules); index++) {
        ASTModuleDeclarationRef module = *((ASTModuleDeclarationRef *)ArrayGetElementAtIndex(sortedModules, index));
        if (module->kind == ASTModuleKindInterface) {
            continue;
        }

        StringRef objectFilePath = StringCreateCopy(workspace->allocator, workspace->buildDirectory);
        StringAppendFormat(objectFilePath, "/%s.o", StringGetCharacters(module->base.name));
        ArrayAppendElement(objectFiles, &objectFilePath);

        StringRef targetPath = StringCreateCopy(workspace->allocator, workspace->buildDirectory);
        StringAppendFormat(targetPath, "/%s", StringGetCharacters(module->base.name));

        ArrayRef linkLibraries       = ArrayCreateEmpty(workspace->allocator, sizeof(StringRef), 0);
        ArrayRef linkFrameworks      = ArrayCreateEmpty(workspace->allocator, sizeof(StringRef), 0);
        ASTArrayIteratorRef iterator = ASTArrayGetIterator(module->linkDirectives);
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

        ArrayDestroy(linkLibraries);
        ArrayDestroy(linkFrameworks);
        StringDestroy(targetPath);
        StringDestroy(objectFilePath);
    }

    ArrayDestroy(objectFiles);
    ArrayDestroy(sortedModules);
    return NULL;
}
