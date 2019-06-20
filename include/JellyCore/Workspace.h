#ifndef __JELLY_WORKSPACE__
#define __JELLY_WORKSPACE__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>
#include <JellyCore/ASTContext.h>

JELLY_EXTERN_C_BEGIN

typedef struct _Workspace *WorkspaceRef;

WorkspaceRef WorkspaceCreate(AllocatorRef allocator, StringRef workingDirectory);

void WorkspaceDestroy(WorkspaceRef workspace);

ASTContextRef WorkspaceGetContext(WorkspaceRef workspace);

void WorkspaceAddSourceFile(WorkspaceRef workspace, StringRef filePath);

Bool WorkspaceStartAsync(WorkspaceRef workspace);

void WorkspaceWaitForFinish(WorkspaceRef workspace);

JELLY_EXTERN_C_END

#endif
