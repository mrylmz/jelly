#include "JellyCore/Array.h"
#include "JellyCore/TempAllocator.h"

struct _TempAllocatorContext {
    AllocatorRef allocator;
    ArrayRef allocations;
};

static inline Bool _IsPointerEqual(const void *elementLeft, const void *elementRight);

void *_AllocatorTemp(AllocatorMode mode, Index capacity, void *memory, void *context);

AllocatorRef TempAllocatorCreate(AllocatorRef allocator) {
    struct _TempAllocatorContext *context = AllocatorAllocate(allocator, sizeof(struct _TempAllocatorContext));
    context->allocator                    = allocator;
    context->allocations                  = ArrayCreateEmpty(allocator, sizeof(void *), 8);
    return AllocatorCreate(allocator, &_AllocatorTemp, context);
}

void *_AllocatorTemp(AllocatorMode mode, Index capacity, void *memory, void *context) {
    struct _TempAllocatorContext *tempContext = context;
    assert(tempContext);

    switch (mode) {
    case AllocatorModeAllocate: {
        void *memory = AllocatorAllocate(tempContext->allocator, capacity);
        assert(memory);
        ArrayAppendElement(tempContext->allocations, &memory);
        return memory;
    }

    case AllocatorModeReallocate: {
        void *newMemory = AllocatorReallocate(tempContext->allocator, memory, capacity);
        if (newMemory) {
            Index index = ArrayGetIndexOfElement(tempContext->allocations, &_IsPointerEqual, memory);
            if (index != kArrayElementNotFound) {
                ArraySetElementAtIndex(tempContext->allocations, index, &newMemory);
            }
        }
        return newMemory;
    }

    case AllocatorModeDeallocate:
        return NULL;

    case AllocatorModeDestroy: {
        for (Index index = 0; index < ArrayGetElementCount(tempContext->allocations); index++) {
            void *memory = *((void **)ArrayGetElementAtIndex(tempContext->allocations, index));
            AllocatorDeallocate(tempContext->allocator, memory);
        }

        ArrayDestroy(tempContext->allocations);
        AllocatorDeallocate(tempContext->allocator, tempContext);
        return NULL;
    }

    default:
        JELLY_UNREACHABLE("Invalid value for mode!");
    }
}

static inline Bool _IsPointerEqual(const void *elementLeft, const void *elementRight) {
    return *((void **)elementLeft) == elementRight;
}
