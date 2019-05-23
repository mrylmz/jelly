#include "JellyCore/Allocator.h"

// @Todo: BumpAllocator will always fail allocation if user-code requests capacities higher than kBumpAllocatorPageCapacity!

const Index kBumpAllocatorPageCapacity = 65535;

struct _BumpAllocatorPage {
    struct _BumpAllocatorPage *next;
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

    Index pageCapacity              = sizeof(struct _BumpAllocatorPage) + sizeof(UInt8) * kBumpAllocatorPageCapacity;
    struct _BumpAllocatorPage *page = AllocatorAllocate(allocator, pageCapacity);
    page->next                      = NULL;
    page->index                     = 0;
    page->memory                    = (UInt8 *)page + sizeof(struct _BumpAllocatorPage);
    context->firstPage              = page;
    context->currentPage            = page;

    return AllocatorCreate(allocator, &_AllocatorBump, context);
}

void *_AllocatorBump(AllocatorMode mode, Index capacity, void *memory, void *context) {
    struct _BumpAllocatorContext *bumpContext = context;
    assert(bumpContext);

    switch (mode) {
    case AllocatorModeAllocate: {
        if (bumpContext->currentPage->index + capacity > kBumpAllocatorPageCapacity) {
            Index pageCapacity              = sizeof(struct _BumpAllocatorPage) + sizeof(UInt8) * kBumpAllocatorPageCapacity;
            struct _BumpAllocatorPage *page = AllocatorAllocate(bumpContext->allocator, pageCapacity);
            page->next                      = NULL;
            page->index                     = 0;
            page->memory                    = (UInt8 *)page + sizeof(struct _BumpAllocatorPage);
            bumpContext->currentPage->next  = page;
            bumpContext->currentPage        = page;
        }

        void *memory = bumpContext->currentPage->memory + sizeof(UInt8) * bumpContext->currentPage->index;
        bumpContext->currentPage->index += capacity;
        return memory;
    }

    case AllocatorModeReallocate:
        return memory;

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
