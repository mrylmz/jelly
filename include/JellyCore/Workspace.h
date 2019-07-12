#ifndef __JELLY_WORKSPACE__
#define __JELLY_WORKSPACE__

#include <JellyCore/ASTContext.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

enum _WorkspaceOptions {
    WorkspaceOptionsNone      = 0,
    WorkspaceOptionsDumpAST   = 1 << 0,
    WorkspaceOptionsDumpScope = 1 << 1,
    WorkspaceOptionsDumpIR    = 1 << 2,
};
typedef enum _WorkspaceOptions WorkspaceOptions;

typedef struct _Workspace *WorkspaceRef;

WorkspaceRef WorkspaceCreate(AllocatorRef allocator, StringRef workingDirectory, WorkspaceOptions options);

void WorkspaceDestroy(WorkspaceRef workspace);

ASTContextRef WorkspaceGetContext(WorkspaceRef workspace);

void WorkspaceAddSourceFile(WorkspaceRef workspace, StringRef filePath);

void WorkspaceSetDumpASTOutput(WorkspaceRef workspace, FILE *output);

void WorkspaceSetDumpScopeOutput(WorkspaceRef workspace, FILE *output);

Bool WorkspaceStartAsync(WorkspaceRef workspace);

void WorkspaceWaitForFinish(WorkspaceRef workspace);

JELLY_EXTERN_C_END

#endif
