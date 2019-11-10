#ifndef __JELLY_BUCKETARRAY__
#define __JELLY_BUCKETARRAY__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _BucketArray *BucketArrayRef;

typedef Bool (*BucketArrayPredicate)(const void *elementLeft, const void *elementRight);

BucketArrayRef BucketArrayCreate(AllocatorRef allocator, Index pageSize, Index elementSize, const void *elements, Index elementCount);

BucketArrayRef BucketArrayCreateCopy(AllocatorRef allocator, Index pageSize, BucketArrayRef array);

BucketArrayRef BucketArrayCreateEmpty(AllocatorRef allocator, Index elementSize, Index capacity);

void BucketArrayDestroy(BucketArrayRef array);

Index BucketArrayGetElementSize(BucketArrayRef array);

Index BucketArrayGetElementCount(BucketArrayRef array);

void *BucketArrayGetElementAtIndex(BucketArrayRef array, Index index);

void BucketArrayCopyElementAtIndex(BucketArrayRef array, Index index, void *element);

void BucketArrayAppendElement(BucketArrayRef array, const void *element);

void *BucketArrayAppendUninitializedElement(BucketArrayRef array);

void BucketArraySetElementAtIndex(BucketArrayRef array, Index index, const void *element);

Bool BucketArrayContainsElement(BucketArrayRef array, BucketArrayPredicate predicate, const void *element);

JELLY_EXTERN_C_END

#endif
