#include "JellyCore/Allocator.h"

const Index kBumpAllocatorDefaultPageCapacity = 65535;

struct _BumpAllocatorPage {
    struct _BumpAllocatorPage *next;
    Index capacity;
    Index index;
    UInt8 *memory;
};

struct _BumpAllocatorContext {
    AllocatorRef allocator;
    struct _BumpAllocatorPage *firstPage;
    struct _BumpAllocatorPage *currentPage;
};

struct _Allocator {
    struct _Allocator *allocator;
    AllocatorCallback callback;
    void *context;
};

void *_AllocatorBump(AllocatorMode mode, Index capacity, void *memory, void *context);

AllocatorRef BumpAllocatorCreate(AllocatorRef allocator) {
    struct _BumpAllocatorContext *context = AllocatorAllocate(allocator, sizeof(struct _BumpAllocatorContext));
    context->allocator                    = allocator;
    context->firstPage                    = NULL;
    context->currentPage                  = NULL;
    return AllocatorCreate(allocator, &_AllocatorBump, context);
}

void *_AllocatorBump(AllocatorMode mode, Index capacity, void *memory, void *context) {
    struct _BumpAllocatorContext *bumpContext = context;
    assert(bumpContext);

    switch (mode) {
    case AllocatorModeAllocate: {
        if (bumpContext->firstPage == NULL) {
            Index memoryCapacity = kBumpAllocatorDefaultPageCapacity;
            while (capacity > memoryCapacity) {
                memoryCapacity *= 2;
            }

            Index pageCapacity              = sizeof(struct _BumpAllocatorPage) + sizeof(UInt8) * memoryCapacity;
            struct _BumpAllocatorPage *page = AllocatorAllocate(bumpContext->allocator, pageCapacity);
            page->next                      = NULL;
            page->capacity                  = memoryCapacity;
            page->index                     = 0;
            page->memory                    = (UInt8 *)page + sizeof(struct _BumpAllocatorPage);
            bumpContext->firstPage          = page;
            bumpContext->currentPage        = page;
        } else if (bumpContext->currentPage->index + capacity > bumpContext->currentPage->capacity) {
            Index memoryCapacity = kBumpAllocatorDefaultPageCapacity;
            while (capacity > memoryCapacity) {
                memoryCapacity *= 2;
            }

            Index pageCapacity              = sizeof(struct _BumpAllocatorPage) + sizeof(UInt8) * memoryCapacity;
            struct _BumpAllocatorPage *page = AllocatorAllocate(bumpContext->allocator, pageCapacity);
            page->next                      = NULL;
            page->capacity                  = memoryCapacity;
            page->index                     = 0;
            page->memory                    = (UInt8 *)page + sizeof(struct _BumpAllocatorPage);
            bumpContext->currentPage->next  = page;
            bumpContext->currentPage        = page;
        }

        void *memory = bumpContext->currentPage->memory + sizeof(UInt8) * bumpContext->currentPage->index;
        bumpContext->currentPage->index += capacity;
        return memory;
    }

    case AllocatorModeReallocate: {
        assert(bumpContext->currentPage);
        void *newMemory = _AllocatorBump(AllocatorModeAllocate, capacity, NULL, context);
        assert(newMemory);
        memcpy(newMemory, memory, capacity);
        return newMemory;
    }

    case AllocatorModeDeallocate:
        return NULL;

    case AllocatorModeDestroy: {
        struct _BumpAllocatorPage *page = bumpContext->firstPage;
        while (page) {
            struct _BumpAllocatorPage *next = page->next;
            AllocatorDeallocate(bumpContext->allocator, page);
            page = next;
        }

        AllocatorDeallocate(bumpContext->allocator, bumpContext);
        return NULL;
    }

    default:
        JELLY_UNREACHABLE("Invalid value for mode!");
    }
}
