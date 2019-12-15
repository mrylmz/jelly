#ifndef __JELLY_DEPENDENCYGRAPH__
#define __JELLY_DEPENDENCYGRAPH__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>
#include <JellyCore/String.h>

JELLY_EXTERN_C_BEGIN

typedef struct _DependencyGraph *DependencyGraphRef;
typedef struct _DependencyGraphNode *DependencyGraphNodeRef;

DependencyGraphRef DependencyGraphCreate(AllocatorRef allocator);

void DependencyGraphDestroy(DependencyGraphRef graph);

DependencyGraphNodeRef DependencyGraphLookupNode(DependencyGraphRef graph, StringRef identifier);

DependencyGraphNodeRef DependencyGraphInsertNode(DependencyGraphRef graph, StringRef identifier);

Bool DependencyGraphAddDependency(DependencyGraphRef graph, DependencyGraphNodeRef node, StringRef identifier);

ArrayRef DependencyGraphGetIdentifiersInTopologicalOrder(DependencyGraphRef graph, Bool *hasCyclicDependencies);

JELLY_EXTERN_C_END

#endif
