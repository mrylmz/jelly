#include "JellyCore/Diagnostic.h"
#include "JellyCore/Parser.h"
#include "JellyCore/Queue.h"
#include "JellyCore/Workspace.h"

#include <pthread.h>

// TODO: Add configuration for ast dump command...
// TODO: Fix memory leaks ...

struct _Workspace {
    AllocatorRef allocator;
    StringRef workingDirectory;
    ArrayRef sourceFilePaths;
    ASTContextRef context;
    ParserRef parser;
    QueueRef parseQueue;

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

WorkspaceRef WorkspaceCreate(AllocatorRef allocator, StringRef workingDirectory) {
    WorkspaceRef workspace      = AllocatorAllocate(allocator, sizeof(struct _Workspace));
    workspace->allocator        = allocator;
    workspace->workingDirectory = StringCreateCopy(allocator, workingDirectory);
    workspace->sourceFilePaths  = ArrayCreateEmpty(allocator, sizeof(StringRef *), 8);
    workspace->context          = ASTContextCreate(allocator);
    workspace->parser           = ParserCreate(allocator, workspace->context);
    workspace->parseQueue       = QueueCreate(allocator);
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
        ReportError("Cannot load same source file twice");
        StringDestroy(absoluteFilePath);
        return;
    }

    ArrayAppendElement(workspace->sourceFilePaths, &absoluteFilePath);

    StringRef copy = StringCreateCopy(workspace->allocator, filePath);
    pthread_mutex_lock(&workspace->mutex);
    QueueEnqueue(workspace->parseQueue, copy);
    pthread_mutex_unlock(&workspace->mutex);
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
    ASTLinkedListRef next = sourceUnit->declarations;
    while (next) {
        if (next->node->tag == ASTTagLoadDirective) {
            ASTLoadDirectiveRef load = (ASTLoadDirectiveRef)next->node;
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
                ReportError("Cannot load same source file twice");
                StringDestroy(relativeFilePath);
                StringDestroy(absoluteFilePath);
            } else {
                ArrayAppendElement(workspace->sourceFilePaths, &absoluteFilePath);
                pthread_mutex_lock(&workspace->mutex);
                QueueEnqueue(workspace->parseQueue, relativeFilePath);
                pthread_mutex_unlock(&workspace->mutex);
            }
        }

        next = next->next;
    }
}

void *_WorkspaceProcess(void *context) {
    WorkspaceRef workspace = (WorkspaceRef)context;

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

    return NULL;
}
