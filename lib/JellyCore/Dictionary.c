#include "JellyCore/Dictionary.h"

const Index _kDictionaryBufferDefaultCapacity = 65535;
const Float32 _kDictionaryBufferGrowthFactor  = 1.5;

struct _DictionaryBucket {
    UInt64 hash;
    Index keyOffset;
    Index elementOffset;

    // NOTE: When ever a bucket is filled the next pointer will guaranteed to be reserved.
    //       As long as next is NULL we assume that the bucket is empty.
    struct _DictionaryBucket *next;
};
typedef struct _DictionaryBucket *DictionaryBucketRef;

struct _DictionaryBuffer {
    Index offset;
    Index capacity;
    void *memory;
};
typedef struct _DictionaryBuffer DictionaryBuffer;

struct _Dictionary {
    AllocatorRef allocator;
    DictionaryKeyComparator comparator;
    DictionaryKeyHasher hasher;
    DictionaryKeySizeCallback keySizeCallback;
    DictionaryBuffer keyBuffer;
    DictionaryBuffer elementBuffer;
    Index capacity;
    Index elementCount;
    DictionaryBucketRef *buckets;
};

static inline void _DictionaryBufferInit(DictionaryRef dictionary, DictionaryBuffer *buffer);
static inline void _DictionaryBufferReserveCapacity(DictionaryRef dictionary, DictionaryBuffer *buffer, Index capacity);
static inline void *_DictionaryBufferGetElement(DictionaryRef dictionary, DictionaryBuffer *buffer, Index offset);
static inline Index _DictionaryBufferInsertElement(DictionaryRef dictionary, DictionaryBuffer *buffer, const void *element,
                                                   Index elementSize);
static inline void _DictionaryBufferDeinit(DictionaryRef dictionary, DictionaryBuffer *buffer);

Bool _CStringDictionaryKeyComparator(const void *lhs, const void *rhs);
UInt64 _CStringDictionaryKeyHasher(const void *key);
void *_CStringDictionaryKeySizeCallback(const void *key);

DictionaryRef DictionaryCreate(AllocatorRef allocator, DictionaryKeyComparator comparator, DictionaryKeyHasher hasher,
                               DictionaryKeySizeCallback keySizeCallback, Index capacity) {
    DictionaryRef dictionary    = (DictionaryRef)AllocatorAllocate(allocator,
                                                                sizeof(struct _Dictionary) + sizeof(struct _DictionaryBucket) * capacity);
    dictionary->allocator       = allocator;
    dictionary->comparator      = comparator;
    dictionary->hasher          = hasher;
    dictionary->keySizeCallback = keySizeCallback;
    dictionary->capacity        = capacity;
    dictionary->elementCount    = 0;
    dictionary->buckets         = (DictionaryBucketRef *)(((UInt8 *)dictionary) + sizeof(struct _Dictionary));
    memset(dictionary->buckets, 0, sizeof(struct _DictionaryBucket) * capacity);
    _DictionaryBufferInit(dictionary, &dictionary->keyBuffer);
    _DictionaryBufferInit(dictionary, &dictionary->elementBuffer);
    return dictionary;
}

DictionaryRef CStringDictionaryCreate(AllocatorRef allocator, Index capacity) {
    return DictionaryCreate(allocator, &_CStringDictionaryKeyComparator, &_CStringDictionaryKeyHasher, &_CStringDictionaryKeySizeCallback,
                            capacity);
}

void DictionaryDestroy(DictionaryRef dictionary) {
    for (Index index = 0; index < dictionary->capacity; index++) {
        DictionaryBucketRef bucket = (DictionaryBucketRef)((UInt8 *)dictionary->buckets + sizeof(struct _DictionaryBucket));
        DictionaryBucketRef next   = bucket->next;
        while (next) {
            DictionaryBucketRef current = next;
            next                        = next->next;
            AllocatorDeallocate(dictionary->allocator, current);
        }
    }

    _DictionaryBufferDeinit(dictionary, &dictionary->elementBuffer);
    _DictionaryBufferDeinit(dictionary, &dictionary->keyBuffer);
    AllocatorDeallocate(dictionary->allocator, dictionary);
}

void DictionaryInsert(DictionaryRef dictionary, const void *key, const void *element, Index elementSize) {
    if (element == NULL || elementSize < 1) {
        return DictionaryRemove(dictionary, key);
    }

    UInt64 hash                = dictionary->hasher(key);
    Index index                = hash % dictionary->capacity;
    DictionaryBucketRef bucket = (DictionaryBucketRef)(((UInt8 *)dictionary->buckets) + sizeof(struct _DictionaryBucket) * index);

    while (bucket->next != NULL) {
        const void *bucketKey = _DictionaryBufferGetElement(dictionary, &dictionary->keyBuffer, bucket->keyOffset);
        if (bucket->hash == hash && dictionary->comparator(bucketKey, key)) {
            // TODO: Remove old element from buffer and update all indices in buckets
            bucket->elementOffset = _DictionaryBufferInsertElement(dictionary, &dictionary->elementBuffer, element, elementSize);
            return;
        }

        bucket = bucket->next;
    }

    bucket->hash          = hash;
    bucket->keyOffset     = _DictionaryBufferInsertElement(dictionary, &dictionary->keyBuffer, key, dictionary->keySizeCallback(key));
    bucket->elementOffset = _DictionaryBufferInsertElement(dictionary, &dictionary->elementBuffer, element, elementSize);
    bucket->next          = (DictionaryBucketRef)AllocatorAllocate(dictionary->allocator, sizeof(struct _DictionaryBucket));
    memset(bucket->next, 0, sizeof(struct _DictionaryBucket));

    dictionary->elementCount += 1;
}

const void *DictionaryLookup(DictionaryRef dictionary, const void *key) {
    UInt64 hash                = dictionary->hasher(key);
    Index index                = hash % dictionary->capacity;
    DictionaryBucketRef bucket = (DictionaryBucketRef)(((UInt8 *)dictionary->buckets) + sizeof(struct _DictionaryBucket) * index);

    while (bucket->next != NULL) {
        const void *bucketKey = _DictionaryBufferGetElement(dictionary, &dictionary->keyBuffer, bucket->keyOffset);
        if (bucket->hash == hash && dictionary->comparator(bucketKey, key)) {
            return _DictionaryBufferGetElement(dictionary, &dictionary->elementBuffer, bucket->elementOffset);
        }

        bucket = bucket->next;
    }

    return NULL;
}

void DictionaryRemove(DictionaryRef dictionary, const void *key) {
    UInt64 hash                        = dictionary->hasher(key);
    Index index                        = hash % dictionary->capacity;
    DictionaryBucketRef bucket         = (DictionaryBucketRef)(((UInt8 *)dictionary->buckets) + sizeof(struct _DictionaryBucket) * index);
    DictionaryBucketRef previousBucket = NULL;

    while (bucket->next) {
        const void *bucketKey = _DictionaryBufferGetElement(dictionary, &dictionary->keyBuffer, bucket->keyOffset);
        if (bucket->hash == hash && dictionary->comparator(bucketKey, key)) {
            if (previousBucket == NULL) {
                memcpy((DictionaryBucketRef)(((UInt8 *)dictionary->buckets) + sizeof(struct _DictionaryBucket) * index), bucket->next,
                       sizeof(struct _DictionaryBucket));
            } else {
                previousBucket->next = bucket->next;
                AllocatorDeallocate(dictionary->allocator, bucket);
            }

            // TODO: Remove key from buffer and update all indices in buckets

            dictionary->elementCount -= 1;
            return;
        }

        previousBucket = bucket;
        bucket         = bucket->next;
    }
}

static inline void _DictionaryBufferInit(DictionaryRef dictionary, DictionaryBuffer *buffer) {
    buffer->offset   = 0;
    buffer->capacity = _kDictionaryBufferDefaultCapacity;
    buffer->memory   = AllocatorAllocate(dictionary->allocator, buffer->capacity);
}

static inline void _DictionaryBufferReserveCapacity(DictionaryRef dictionary, DictionaryBuffer *buffer, Index capacity) {
    Index newCapacity = buffer->capacity;
    while (newCapacity < capacity) {
        newCapacity *= _kDictionaryBufferGrowthFactor;
    }

    if (newCapacity > buffer->capacity) {
        buffer->capacity = newCapacity;
        buffer->memory   = AllocatorReallocate(dictionary->allocator, buffer->memory, buffer->capacity);
    }
}

static inline void *_DictionaryBufferGetElement(DictionaryRef dictionary, DictionaryBuffer *buffer, Index offset) {
    return (void *)(((UInt8 *)buffer->memory) + offset);
}

static inline Index _DictionaryBufferInsertElement(DictionaryRef dictionary, DictionaryBuffer *buffer, const void *element,
                                                   Index elementSize) {
    Index requiredCapacity = buffer->offset + elementSize;
    _DictionaryBufferReserveCapacity(dictionary, buffer, requiredCapacity);
    Index offset       = buffer->offset;
    UInt8 *destination = ((UInt8 *)buffer->memory) + buffer->offset;
    memcpy(destination, element, elementSize);
    buffer->offset += elementSize;
    return offset;
}

static inline void _DictionaryBufferDeinit(DictionaryRef dictionary, DictionaryBuffer *buffer) {
    AllocatorDeallocate(dictionary->allocator, buffer->memory);
}

Bool _CStringDictionaryKeyComparator(const void *lhs, const void *rhs) {
    return strcmp((const char *)lhs, (const char *)rhs) == 0;
}

UInt64 _CStringDictionaryKeyHasher(const void *key) {
    UInt64 hash         = 5381;
    const char *current = (const char *)key;

    while (*current != '\0') {
        hash = hash * 33 + (*current);
        current += 1;
    }

    return hash;
}

void *_CStringDictionaryKeySizeCallback(const void *key) {
    return strlen((const char *)key) + 1;
}
