#include "JellyCore/Array.h"

struct _Array {
    AllocatorRef allocator;
    Index elementSize;
    Index elementCount;
    Index capacity;
    UInt8 *memory;
};

void _ArrayReserveCapacity(ArrayRef array, Index capacity);

ArrayRef ArrayCreate(AllocatorRef allocator, Index elementSize, const void *elements, Index elementCount) {
    ArrayRef array = AllocatorAllocate(allocator, sizeof(struct _Array));
    assert(array);
    array->allocator    = allocator;
    array->elementSize  = elementSize;
    array->elementCount = elementCount;
    array->capacity     = elementCount;
    array->memory       = AllocatorAllocate(allocator, elementSize * elementCount);
    assert(array->memory);
    memcpy(array->memory, elements, elementSize * elementCount);
    return array;
}

ArrayRef ArrayCreateCopy(AllocatorRef allocator, ArrayRef array) {
    return ArrayCreate(allocator, array->elementSize, array->memory, array->elementCount);
}

ArrayRef ArrayCreateEmpty(AllocatorRef allocator, Index elementSize, Index capacity) {
    ArrayRef array = AllocatorAllocate(allocator, sizeof(struct _Array) + elementSize * capacity);
    assert(array);
    array->allocator    = allocator;
    array->elementSize  = elementSize;
    array->elementCount = 0;
    array->capacity     = capacity;
    array->memory       = AllocatorAllocate(allocator, elementSize * capacity);
    assert(array->memory);
    return array;
}

void ArrayDestroy(ArrayRef array) {
    AllocatorDeallocate(array->allocator, array->memory);
    AllocatorDeallocate(array->allocator, array);
}

Index ArrayGetElementSize(ArrayRef array) {
    return array->elementSize;
}

Index ArrayGetElementCount(ArrayRef array) {
    return array->elementCount;
}

Index ArrayGetCapacity(ArrayRef array) {
    return array->capacity;
}

Index ArrayGetSortedInsertionIndex(ArrayRef array, ArrayPredicate isOrderedAscending, void *element) {
    if (ArrayGetElementCount(array) < 1) {
        return 0;
    }

    Index index = ArrayGetElementCount(array);
    while (0 < index && isOrderedAscending(element, ArrayGetElementAtIndex(array, index - 1))) {
        index -= 1;
    }

    return index;
}

void *ArrayGetElementAtIndex(ArrayRef array, Index index) {
    assert(index < array->elementCount);
    return array->memory + array->elementSize * index;
}

void ArrayCopyElementAtIndex(ArrayRef array, Index index, void *element) {
    assert(index < array->elementCount);
    memcpy(element, ArrayGetElementAtIndex(array, index), array->elementSize);
}

void ArrayAppendElement(ArrayRef array, const void *element) {
    _ArrayReserveCapacity(array, array->elementCount + 1);
    memcpy(array->memory + array->elementSize * array->elementCount, element, array->elementSize);
    array->elementCount += 1;
}

void *ArrayAppendUninitializedElement(ArrayRef array) {
    _ArrayReserveCapacity(array, array->elementCount + 1);
    array->elementCount += 1;
    return ArrayGetElementAtIndex(array, array->elementCount - 1);
}

void ArrayAppendArray(ArrayRef array, ArrayRef other) {
    assert(array->elementSize == other->elementSize);

    for (Index index = 0; index < other->elementCount; index++) {
        ArrayAppendElement(array, ArrayGetElementAtIndex(other, index));
    }
}

void ArrayInsertElementAtIndex(ArrayRef array, Index index, const void *element) {
    if (index == array->elementCount) {
        return ArrayAppendElement(array, element);
    }

    assert(index < array->elementCount);
    _ArrayReserveCapacity(array, array->elementCount + 1);

    Index tailLength = array->elementCount - index;
    if (tailLength > 0) {
        void *source      = ArrayGetElementAtIndex(array, index);
        void *destination = ArrayGetElementAtIndex(array, index + 1);
        memmove(destination, source, array->elementSize * tailLength);
    }

    memcpy(ArrayGetElementAtIndex(array, index), element, array->elementSize);
    array->elementCount += 1;
}

void ArraySetElementAtIndex(ArrayRef array, Index index, const void *element) {
    assert(index < array->elementCount);
    memcpy(ArrayGetElementAtIndex(array, index), element, array->elementSize);
}

void ArrayRemoveElementAtIndex(ArrayRef array, Index index) {
    assert(index < array->elementCount);

    Index tailLength = array->elementCount - index - 1;
    if (tailLength > 0) {
        void *source      = ArrayGetElementAtIndex(array, index + 1);
        void *destination = ArrayGetElementAtIndex(array, index);
        memmove(destination, source, array->elementSize * tailLength);
    }

    array->elementCount -= 1;
}

void ArrayRemoveAllElements(ArrayRef array, Bool keepCapacity) {
    array->elementCount = 0;

    if (!keepCapacity) {
        Index newCapacity = array->capacity > 8 ? 8 : array->capacity;
        UInt8 *newMemory  = AllocatorReallocate(array->allocator, array->memory, newCapacity);
        assert(newMemory);
        array->capacity = newCapacity;
        array->memory   = newMemory;
    }
}

bool ArrayContainsElement(ArrayRef array, ArrayPredicate predicate, const void *element) {
    for (Index index = 0; index < ArrayGetElementCount(array); index++) {
        void *lhs = ArrayGetElementAtIndex(array, index);
        if (predicate(lhs, element)) {
            return true;
        }
    }

    return false;
}

bool ArrayIsEqual(ArrayRef lhs, ArrayRef rhs) {
    if (lhs->elementSize != rhs->elementSize || lhs->elementCount != rhs->elementCount) {
        return false;
    }

    return memcmp(lhs->memory, rhs->memory, lhs->elementSize * lhs->elementCount) == 0;
}

void _ArrayReserveCapacity(ArrayRef array, Index capacity) {
    Index newCapacity = array->capacity;
    while (newCapacity < capacity) {
        newCapacity *= 2;
    }

    if (array->capacity < newCapacity) {
        UInt8 *newMemory = AllocatorReallocate(array->allocator, array->memory, array->elementSize * newCapacity);
        assert(newMemory);
        array->capacity = newCapacity;
        array->memory   = newMemory;
    }
}
