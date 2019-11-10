#include "JellyCore/Array.h"
#include "JellyCore/Queue.h"

struct _Queue {
    AllocatorRef allocator;
    ArrayRef elements;
};

QueueRef QueueCreate(AllocatorRef allocator) {
    QueueRef queue   = AllocatorAllocate(allocator, sizeof(struct _Queue));
    queue->allocator = allocator;
    queue->elements  = ArrayCreateEmpty(allocator, sizeof(void *), 8);
    return queue;
}

void QueueDestroy(QueueRef queue) {
    ArrayDestroy(queue->elements);
    AllocatorDeallocate(queue->allocator, queue);
}

void QueueEnqueue(QueueRef queue, void *element) {
    ArrayAppendElement(queue->elements, &element);
}

void *QueueDequeue(QueueRef queue) {
    if (ArrayGetElementCount(queue->elements) < 1) {
        return NULL;
    }

    void *element = *((void **)ArrayGetElementAtIndex(queue->elements, 0));
    ArrayRemoveElementAtIndex(queue->elements, 0);
    return element;
}
