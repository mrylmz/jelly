#ifndef __JELLY_ARRAY__
#define __JELLY_ARRAY__

#include <JellyCore/Base.h>
#include <JellyCore/Allocator.h>

JELLY_EXTERN_C_BEGIN

typedef struct _Array* ArrayRef;

typedef bool (*ArrayPredicate)(const void *elementLeft, const void *elementRight);

ArrayRef ArrayCreate(AllocatorRef allocator, Index elementSize, const void *elements, Index elementCount);

ArrayRef ArrayCreateCopy(AllocatorRef allocator, ArrayRef array);

ArrayRef ArrayCreateEmpty(AllocatorRef allocator, Index elementSize, Index capacity);

void ArrayDestroy(ArrayRef array);

Index ArrayGetElementSize(ArrayRef array);

Index ArrayGetElementCount(ArrayRef array);

Index ArrayGetCapacity(ArrayRef array);

void *ArrayGetElementAtIndex(ArrayRef array, Index index);

void ArrayCopyElementAtIndex(ArrayRef array, Index index, void *element);

void ArrayAppendElement(ArrayRef array, const void *element);

void ArrayInsertElementAtIndex(ArrayRef array, Index index, const void *element);

void ArraySetElementAtIndex(ArrayRef array, Index index, const void *element);

void ArrayRemoveElementAtIndex(ArrayRef array, Index index);

void ArrayRemoveAllElements(ArrayRef array, Bool keepCapacity);

void ArrayContainsElement(ArrayRef array, ArrayPredicate predicate, const void *element);

bool ArrayIsEqual(ArrayRef lhs, ArrayRef rhs);

JELLY_EXTERN_C_END

#endif
