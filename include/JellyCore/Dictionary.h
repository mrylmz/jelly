#ifndef __JELLY_DICTIONARY__
#define __JELLY_DICTIONARY__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef Bool (*DictionaryKeyComparator)(const void *lhs, const void *rhs);
typedef UInt64 (*DictionaryKeyHasher)(const void *key);
typedef void *(*DictionaryKeySizeCallback)(const void *key);

typedef struct _Dictionary *DictionaryRef;

DictionaryRef DictionaryCreate(AllocatorRef allocator, DictionaryKeyComparator comparator, DictionaryKeyHasher hasher,
                               DictionaryKeySizeCallback keySizeCallback, Index capacity);

DictionaryRef CStringDictionaryCreate(AllocatorRef allocator, Index capacity);

void DictionaryDestroy(DictionaryRef dictionary);

void DictionaryInsert(DictionaryRef dictionary, const void *key, const void *element, Index elementSize);

const void *DictionaryLookup(DictionaryRef dictionary, const void *key);

void DictionaryRemove(DictionaryRef dictionary, const void *key);

void DictionaryGetKeyBuffer(DictionaryRef dictionary, void **memory, Index *length);

void DictionaryGetValueBuffer(DictionaryRef dictionary, void **memory, Index *length);

JELLY_EXTERN_C_END

#endif
