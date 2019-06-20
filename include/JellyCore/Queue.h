#ifndef __JELLY_QUEUE__
#define __JELLY_QUEUE__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _Queue *QueueRef;

QueueRef QueueCreate(AllocatorRef allocator);

void QueueDestroy(QueueRef queue);

void QueueEnqueue(QueueRef queue, void *element);

void *QueueDequeue(QueueRef queue);

JELLY_EXTERN_C_END

#endif
