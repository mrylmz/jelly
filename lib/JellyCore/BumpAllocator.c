#include "JellyCore/BumpAllocator.h"

// TODO: Add correct memory alignment and padding!

const Index kBumpAllocatorDefaultPageCapacity = 65535;

struct _BumpAllocatorPage {
    struct _BumpAllocatorPage *next;
    Index capacity;
    Index index;
    UInt8 *memory;
} __attribute__((packed));

struct _BumpAllocatorContext {
    AllocatorRef allocator;
    Index alignment;
    Index pageHeaderSize;
    struct _BumpAllocatorPage *firstPage;
    struct _BumpAllocatorPage *currentPage;
};

struct _Allocator {
    struct _Allocator *allocator;
    AllocatorCallback callback;
    void *context;
};

void *_AllocatorBump(AllocatorMode mode, Index capacity, void *memory, void *context);

static inline Bool _IsPowerOfTwo(Index value);
static inline Index _Align(Index value, Index alignment);

AllocatorRef BumpAllocatorCreate(AllocatorRef allocator) {
    struct _BumpAllocatorContext *context = AllocatorAllocate(allocator, sizeof(struct _BumpAllocatorContext));
    context->allocator                    = allocator;
    context->alignment                    = 2 * sizeof(void *);
    context->pageHeaderSize               = _Align(sizeof(struct _BumpAllocatorPage), context->alignment);
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
            Index memoryCapacity            = MAX(kBumpAllocatorDefaultPageCapacity, capacity + bumpContext->pageHeaderSize);
            memoryCapacity                  = _Align(memoryCapacity, bumpContext->alignment);
            struct _BumpAllocatorPage *page = AllocatorAllocate(bumpContext->allocator, memoryCapacity);
            page->next                      = NULL;
            page->capacity                  = capacity;
            page->index                     = 0;
            page->memory                    = (UInt8 *)page + bumpContext->pageHeaderSize;
            bumpContext->firstPage          = page;
            bumpContext->currentPage        = page;
        } else if (bumpContext->currentPage->index + capacity > bumpContext->currentPage->capacity) {
            Index memoryCapacity            = MAX(kBumpAllocatorDefaultPageCapacity, capacity + bumpContext->pageHeaderSize);
            memoryCapacity                  = _Align(memoryCapacity, bumpContext->alignment);
            struct _BumpAllocatorPage *page = AllocatorAllocate(bumpContext->allocator, memoryCapacity);
            page->next                      = NULL;
            page->capacity                  = capacity;
            page->index                     = 0;
            page->memory                    = (UInt8 *)page + bumpContext->pageHeaderSize;
            bumpContext->currentPage->next  = page;
            bumpContext->currentPage        = page;
        }

        void *memory = bumpContext->currentPage->memory + sizeof(UInt8) * bumpContext->currentPage->index;
        bumpContext->currentPage->index += capacity;
        return memory;
    }

    case AllocatorModeReallocate: {
        // TODO: This is a workaround for now and should be refined or removed entirely.
        //       A reallocation shouldn't be required here but currently it is because the BumpAllocator
        //       is not used explicitly in context due to the abstraction of Allocator!
        //       Rethink the structure and usage of the allocator API!!!
        assert(bumpContext->currentPage);
        struct _BumpAllocatorPage *page = bumpContext->firstPage;
        void *newMemory                 = NULL;
        while (page) {
            Index start  = (Index)page;
            Index end    = start + page->capacity;
            Index offset = (Index)memory;
            if (start <= offset && offset < end) {
                Index newCapacity = MAX(capacity, page->capacity);
                newMemory         = _AllocatorBump(AllocatorModeAllocate, newCapacity, NULL, context);
                memmove(newMemory, memory, MIN(capacity, page->capacity));
            }

            page = page->next;
        }

        assert(newMemory);
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

static inline Bool _IsPowerOfTwo(Index value) {
    return (value & (value - 1)) == 0;
}

static inline Index _Align(Index value, Index alignment) {
    assert(_IsPowerOfTwo(alignment));

    return (value + alignment - 1) & ~(alignment - 1);
}
