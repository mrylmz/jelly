#include "JellyCore/BucketArray.h"

struct _BucketArrayPage {
    struct _BucketArrayPage *next;
    Index index;
    UInt8 *memory;
} __attribute__((packed));

struct _BucketArray {
    AllocatorRef allocator;
    Index pageSize;
    Index alignedPageSize;
    Index elementSize;
    Index elementCountPerPage;
    Index elementCount;
    struct _BucketArrayPage *firstPage;
    struct _BucketArrayPage *currentPage;
};

static inline Bool _IsPowerOfTwo(Index value);
static inline Index _Align(Index value, Index alignment);

static inline struct _BucketArrayPage *_BucketArrayPageCreate(AllocatorRef allocator, Index capacity);

BucketArrayRef BucketArrayCreate(AllocatorRef allocator, Index pageSize, Index elementSize, const void *elements, Index elementCount) {
    BucketArrayRef array = BucketArrayCreateEmpty(allocator, pageSize, elementSize);

    for (Index index = 0; index < elementCount; index++) {
        const UInt8 *element = (UInt8 *)elements + elementSize * index;
        BucketArrayAppendElement(array, element);
    }

    return array;
}

BucketArrayRef BucketArrayCreateCopy(AllocatorRef allocator, Index pageSize, BucketArrayRef array) {
    BucketArrayRef copy = BucketArrayCreateEmpty(allocator, pageSize, array->elementSize);

    struct _BucketArrayPage *page = array->firstPage;
    while (page) {
        for (Index index = 0; index < page->index; index++) {
            const UInt8 *element = page->memory + array->elementSize * index;
            BucketArrayAppendElement(copy, element);
        }

        page = page->next;
    }

    return copy;
}

BucketArrayRef BucketArrayCreateEmpty(AllocatorRef allocator, Index elementSize, Index capacity) {
    BucketArrayRef array       = AllocatorAllocate(allocator, sizeof(struct _BucketArray));
    array->allocator           = allocator;
    array->pageSize            = elementSize * capacity;
    array->alignedPageSize     = _Align(array->pageSize, 2 * sizeof(void *));
    array->elementSize         = elementSize;
    array->elementCountPerPage = array->alignedPageSize / array->elementSize;
    array->elementCount        = 0;
    array->firstPage           = _BucketArrayPageCreate(allocator, array->alignedPageSize);
    array->currentPage         = array->firstPage;
    return array;
}

void BucketArrayDestroy(BucketArrayRef array) {
    struct _BucketArrayPage *page = array->firstPage;
    while (page) {
        struct _BucketArrayPage *next = page->next;
        AllocatorDeallocate(array->allocator, page);
        page = next;
    }

    AllocatorDeallocate(array->allocator, array);
}

Index BucketArrayGetElementSize(BucketArrayRef array) {
    return array->elementSize;
}

Index BucketArrayGetElementCount(BucketArrayRef array) {
    return array->elementCount;
}

void *BucketArrayGetElementAtIndex(BucketArrayRef array, Index index) {
    Index pageIndex  = index / array->elementCountPerPage;
    Index pageOffset = index % array->elementCountPerPage;

    struct _BucketArrayPage *page = array->firstPage;
    Index currentPageIndex        = 0;
    while (currentPageIndex < pageIndex) {
        assert(page->next);
        page = page->next;
        currentPageIndex += 1;
    }

    assert(pageOffset < page->index);
    return page->memory + array->elementSize * pageOffset;
}

void BucketArrayCopyElementAtIndex(BucketArrayRef array, Index index, void *element) {
    void *source = BucketArrayGetElementAtIndex(array, index);
    memcpy(element, source, array->elementSize);
}

void BucketArrayAppendElement(BucketArrayRef array, const void *element) {
    void *destination = BucketArrayAppendUninitializedElement(array);
    memcpy(destination, element, array->elementSize);
}

void *BucketArrayAppendUninitializedElement(BucketArrayRef array) {
    if (array->currentPage->index >= array->elementCountPerPage) {
        struct _BucketArrayPage *page = _BucketArrayPageCreate(array->allocator, array->alignedPageSize);
        array->currentPage->next      = page;
        array->currentPage            = page;
    }

    assert(array->currentPage->index < array->elementCountPerPage);
    UInt8 *element = array->currentPage->memory + array->currentPage->index * array->elementSize;
    array->currentPage->index += 1;
    array->elementCount += 1;
    return element;
}

void BucketArraySetElementAtIndex(BucketArrayRef array, Index index, const void *element) {
    void *destination = BucketArrayGetElementAtIndex(array, index);
    memcpy(destination, element, array->elementSize);
}

Bool BucketArrayContainsElement(BucketArrayRef array, BucketArrayPredicate predicate, const void *element) {
    for (Index index = 0; index < BucketArrayGetElementCount(array); index++) {
        void *lhs = BucketArrayGetElementAtIndex(array, index);
        if (predicate(lhs, element)) {
            return true;
        }
    }

    return false;
}

static inline Bool _IsPowerOfTwo(Index value) {
    return (value & (value - 1)) == 0;
}

static inline Index _Align(Index value, Index alignment) {
    assert(_IsPowerOfTwo(alignment));

    return (value + alignment - 1) & ~(alignment - 1);
}

static inline struct _BucketArrayPage *_BucketArrayPageCreate(AllocatorRef allocator, Index capacity) {
    struct _BucketArrayPage *page = AllocatorAllocate(allocator, sizeof(struct _BucketArrayPage) + capacity);
    page->index                   = 0;
    page->memory                  = (UInt8 *)page + sizeof(struct _BucketArrayPage);
    return page;
}
