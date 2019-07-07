#include "JellyCore/ASTDumper.h"
#include "JellyCore/ASTScope.h"
#include "JellyCore/Diagnostic.h"
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
    ArrayRef sourceFilePaths;
    ASTContextRef context;
    ParserRef parser;
    QueueRef parseQueue;

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
void _WorkspaceParse(void *argument, void *context);
void *_WorkspaceProcess(void *context);

WorkspaceRef WorkspaceCreate(AllocatorRef allocator, StringRef workingDirectory, WorkspaceOptions options) {
    WorkspaceRef workspace      = AllocatorAllocate(allocator, sizeof(struct _Workspace));
    workspace->allocator        = allocator;
    workspace->workingDirectory = StringCreateCopy(allocator, workingDirectory);
    workspace->sourceFilePaths  = ArrayCreateEmpty(allocator, sizeof(StringRef *), 8);
    workspace->context          = ASTContextCreate(allocator);
    workspace->parser           = ParserCreate(allocator, workspace->context);
    workspace->parseQueue       = QueueCreate(allocator);
    workspace->options          = options;
    workspace->dumpASTOutput    = stdout;
    workspace->dumpScopeOutput  = stdout;
    workspace->running          = false;
    workspace->waiting          = false;
    return workspace;
}

void WorkspaceDestroy(WorkspaceRef workspace) {
    if (workspace->running && !workspace->waiting) {
        WorkspaceWaitForFinish(workspace);
    }

    ArrayDestroy(workspace->sourceFilePaths);
    QueueDestroy(workspace->parseQueue);
    ParserDestroy(workspace->parser);
    ASTContextDestroy(workspace->context);
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

void *_WorkspaceProcess(void *context) {
    WorkspaceRef workspace = (WorkspaceRef)context;

    // Parse phase
    while (true) {
        pthread_mutex_lock(&workspace->mutex);
        StringRef filePath = QueueDequeue(workspace->parseQueue);
        pthread_mutex_unlock(&workspace->mutex);

        if (filePath) {
            StringRef absoluteFilePath = StringCreateCopy(workspace->allocator, workspace->workingDirectory);
            StringAppend(absoluteFilePath, "/");
            StringAppendString(absoluteFilePath, filePath);
            StringRef source = StringCreateFromFile(workspace->allocator, StringGetCharacters(absoluteFilePath));
            if (source) {
                ASTSourceUnitRef sourceUnit = ParserParseSourceUnit(workspace->parser, filePath, source);
                StringDestroy(source);
                _WorkspacePerformLoads(workspace, sourceUnit);
            } else {
                ReportError("File not found");
            }

            StringDestroy(absoluteFilePath);
            StringDestroy(filePath);
        } else {
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

    // Name resolution phase
    PerformNameResolution(workspace->context, ASTContextGetModule(workspace->context));

    if ((workspace->options & WorkspaceOptionsDumpScope) > 0) {
        ASTScopeDump(ASTContextGetGlobalScope(workspace->context), workspace->dumpScopeOutput);
    }

    TypeCheckerRef typeChecker = TypeCheckerCreate(workspace->allocator);
    TypeCheckerValidateModule(typeChecker, workspace->context, ASTContextGetModule(workspace->context));
    TypeCheckerDestroy(typeChecker);

    return NULL;
}
