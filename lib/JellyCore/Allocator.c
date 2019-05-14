#include "JellyCore/Allocator.h"

struct _Allocator {
    struct _Allocator *allocator;
    AllocatorCallback callback;
    void *context;
};

void *_AllocatorMalloc(AllocatorMode mode, Index capacity, void *memory, void *context);
void *_AllocatorNull(AllocatorMode mode, Index capacity, void *memory, void *context);

AllocatorRef _AllocatorGetDefault(AllocatorRef allocator);
void *_AllocatorInvokeCallback(AllocatorRef allocator, AllocatorMode mode, Index capacity, void *memory);

static struct _Allocator _kAllocatorSystemDefault = { NULL, &_AllocatorMalloc, NULL };
static struct _Allocator _kAllocatorMalloc = { NULL, &_AllocatorMalloc, NULL };
static struct _Allocator _kAllocatorNull = { NULL, &_AllocatorNull, NULL };

const AllocatorRef kAllocatorDefault = NULL;
const AllocatorRef kAllocatorSystemDefault = &_kAllocatorSystemDefault;
const AllocatorRef kAllocatorMalloc = &_kAllocatorMalloc;
const AllocatorRef kAllocatorNull = &_kAllocatorNull;

static AllocatorRef kAllocatorCurrentDefault = kAllocatorDefault;

void AllocatorSetDefault(AllocatorRef allocator) {
    kAllocatorCurrentDefault = allocator;
}

AllocatorRef AllocatorGetDefault() {
    return kAllocatorCurrentDefault;
}

AllocatorRef AllocatorCreate(AllocatorRef allocator, AllocatorCallback callback, void *context) {
    AllocatorRef newAllocator = (AllocatorRef)AllocatorAllocate(allocator, sizeof(struct _Allocator));
    newAllocator->allocator = allocator;
    newAllocator->callback = callback;
    newAllocator->context = context;
    return newAllocator;
}

void AllocatorDestroy(AllocatorRef allocator) {
    assert(allocator->allocator);
    AllocatorDeallocate(allocator->allocator, allocator);
}

void *AllocatorAllocate(AllocatorRef allocator, Index capacity) {
    return _AllocatorInvokeCallback(allocator, AllocatorModeAllocate, capacity, NULL);
}

void *AllocatorReallocate(AllocatorRef allocator, void *memory, Index capacity) {
    return _AllocatorInvokeCallback(allocator, AllocatorModeReallocate, capacity, memory);
}

void *AllocatorDeallocate(AllocatorRef allocator, void *memory) {
    return _AllocatorInvokeCallback(allocator, AllocatorModeDeallocate, 0, memory);
}

void *_AllocatorMalloc(AllocatorMode mode, Index capacity, void *memory, void *context) {
    switch (mode) {
        case AllocatorModeAllocate:
            return malloc(capacity);

        case AllocatorModeReallocate:
            return realloc(memory, capacity);

        case AllocatorModeDeallocate:
            free(memory);
            return NULL;

        default:
            JELLY_UNREACHABLE("Invalid value for mode!");
    }
}

void *_AllocatorNull(AllocatorMode mode, Index capacity, void *memory, void *context) {
    return NULL;
}

AllocatorRef _AllocatorGetDefault(AllocatorRef allocator) {
    if (allocator == kAllocatorDefault) {
        allocator = AllocatorGetDefault();
    }

    if (allocator == NULL) {
        return kAllocatorSystemDefault;
    }

    return allocator;
}

void *_AllocatorInvokeCallback(AllocatorRef allocator, AllocatorMode mode, Index capacity, void *memory) {
    allocator = _AllocatorGetDefault(allocator);
    return allocator->callback(mode, capacity, memory, allocator->context);
}
