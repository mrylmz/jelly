#include "JellyCore/Array.h"
#include "JellyCore/DependencyGraph.h"
#include "JellyCore/Dictionary.h"
#include "JellyCore/Queue.h"

struct _DependencyGraphNode {
    Index index;
    StringRef identifier;
    ArrayRef dependencies;
};

struct _DependencyGraph {
    AllocatorRef allocator;
    DictionaryRef indices;
    ArrayRef nodes;
};

DependencyGraphRef DependencyGraphCreate(AllocatorRef allocator) {
    DependencyGraphRef graph = (DependencyGraphRef)AllocatorAllocate(allocator, sizeof(struct _DependencyGraph));
    graph->allocator         = allocator;
    graph->indices           = CStringDictionaryCreate(allocator, 8);
    graph->nodes             = ArrayCreateEmpty(allocator, sizeof(struct _DependencyGraphNode), 8);
    return graph;
}

void DependencyGraphDestroy(DependencyGraphRef graph) {
    for (Index index = 0; index < ArrayGetElementCount(graph->nodes); index++) {
        DependencyGraphNodeRef node = (DependencyGraphNodeRef)ArrayGetElementAtIndex(graph->nodes, index);
        StringDestroy(node->identifier);
        ArrayDestroy(node->dependencies);
    }

    DictionaryDestroy(graph->indices);
    ArrayDestroy(graph->nodes);
    AllocatorDeallocate(graph->allocator, graph);
}

DependencyGraphNodeRef DependencyGraphLookupNode(DependencyGraphRef graph, StringRef identifier) {
    Index *index = (Index *)DictionaryLookup(graph->indices, StringGetCharacters(identifier));
    if (index == NULL) {
        return NULL;
    }

    return (DependencyGraphNodeRef)ArrayGetElementAtIndex(graph->nodes, *index);
}

DependencyGraphNodeRef DependencyGraphInsertNode(DependencyGraphRef graph, StringRef identifier) {
    if (DictionaryLookup(graph->indices, StringGetCharacters(identifier)) != NULL) {
        return NULL;
    }

    DependencyGraphNodeRef node = ArrayAppendUninitializedElement(graph->nodes);
    node->index                 = ArrayGetElementCount(graph->nodes) - 1;
    node->identifier            = StringCreateCopy(graph->allocator, identifier);
    node->dependencies          = ArrayCreateEmpty(graph->allocator, sizeof(Index), 8);
    DictionaryInsert(graph->indices, StringGetCharacters(identifier), &node->index, sizeof(Index));
    return node;
}

Bool DependencyGraphAddDependency(DependencyGraphRef graph, DependencyGraphNodeRef node, StringRef identifier) {
    DependencyGraphNodeRef dependency = DependencyGraphLookupNode(graph, identifier);
    if (dependency == NULL) {
        return false;
    }

    for (Index index = 0; index < ArrayGetElementCount(node->dependencies); index++) {
        Index dependencyIndex = *((Index *)ArrayGetElementAtIndex(node->dependencies, index));
        if (dependencyIndex == dependency->index) {
            return true;
        }
    }

    ArrayAppendElement(node->dependencies, &dependency->index);
    return true;
}

// TODO: Improve API to collect information about cyclic dependency occurences...
ArrayRef DependencyGraphGetIdentifiersInTopologicalOrder(DependencyGraphRef graph, Bool *hasCyclicDependencies) {
    ArrayRef inDegree = ArrayCreateEmpty(graph->allocator, sizeof(Index), ArrayGetElementCount(graph->nodes));
    for (Index index = 0; index < ArrayGetElementCount(graph->nodes); index++) {
        Index initialValue = 0;
        ArrayAppendElement(inDegree, &initialValue);
    }

    for (Index index = 0; index < ArrayGetElementCount(graph->nodes); index++) {
        DependencyGraphNodeRef node = (DependencyGraphNodeRef)ArrayGetElementAtIndex(graph->nodes, index);
        for (Index dependencyIndex = 0; dependencyIndex < ArrayGetElementCount(node->dependencies); dependencyIndex++) {
            Index dependency = *((Index *)ArrayGetElementAtIndex(node->dependencies, dependencyIndex));
            Index *value     = (Index *)ArrayGetElementAtIndex(inDegree, dependency);
            *value += 1;
        }
    }

    QueueRef queue = QueueCreate(graph->allocator);
    for (Index index = 0; index < ArrayGetElementCount(graph->nodes); index++) {
        Index nodeInDegree = *((Index *)ArrayGetElementAtIndex(inDegree, index));
        if (nodeInDegree == 0) {
            QueueEnqueue(queue, ArrayGetElementAtIndex(graph->nodes, index));
        }
    }

    Index visitedNodeCount = 0;
    ArrayRef result        = ArrayCreateEmpty(graph->allocator, sizeof(StringRef), ArrayGetElementCount(graph->nodes));
    while (true) {
        DependencyGraphNodeRef node = (DependencyGraphNodeRef)QueueDequeue(queue);
        if (!node) {
            break;
        }

        ArrayAppendElement(result, &node->identifier);

        for (Index index = 0; index < ArrayGetElementCount(node->dependencies); index++) {
            Index dependency = *((Index *)ArrayGetElementAtIndex(node->dependencies, index));
            Index *value     = (Index *)ArrayGetElementAtIndex(inDegree, dependency);
            *value -= 1;

            if (*value <= 0) {
                QueueEnqueue(queue, ArrayGetElementAtIndex(graph->nodes, dependency));
            }
        }

        visitedNodeCount += 1;
    }

    *hasCyclicDependencies = visitedNodeCount != ArrayGetElementCount(graph->nodes);

    QueueDestroy(queue);
    ArrayDestroy(inDegree);
    return result;
}
