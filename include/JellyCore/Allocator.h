#ifndef __JELLY_ALLOCATOR__
#define __JELLY_ALLOCATOR__

#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef enum {
    AllocatorModeAllocate,
    AllocatorModeReallocate,
    AllocatorModeDeallocate,
    AllocatorModeDestroy,
} AllocatorMode;

typedef void *(*AllocatorCallback)(AllocatorMode mode, Index capacity, void *memory, void *context);

typedef struct _Allocator *AllocatorRef;

const AllocatorRef kAllocatorDefault;
const AllocatorRef kAllocatorSystemDefault;
const AllocatorRef kAllocatorMalloc;
const AllocatorRef kAllocatorNull;

void AllocatorSetDefault(AllocatorRef allocator);

AllocatorRef AllocatorGetDefault();

AllocatorRef AllocatorCreate(AllocatorRef allocator, AllocatorCallback callback, void *context);

void AllocatorDestroy(AllocatorRef allocator);

void *AllocatorAllocate(AllocatorRef allocator, Index capacity);

void *AllocatorReallocate(AllocatorRef allocator, void *memory, Index capacity);

void *AllocatorDeallocate(AllocatorRef allocator, void *memory);

JELLY_EXTERN_C_END

#endif
