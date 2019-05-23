#include "JellyCore/String.h"

struct _String {
    AllocatorRef allocator;
    Index length;
    Char *memory;
};

StringRef StringCreate(AllocatorRef allocator, const Char *rawString) {
    StringRef string = AllocatorAllocate(allocator, sizeof(struct _String));
    assert(string);
    string->allocator = allocator;
    string->length    = strlen(rawString);
    string->memory    = AllocatorAllocate(allocator, sizeof(Char) * string->length + 1);
    assert(string->memory);
    memcpy(string->memory, rawString, sizeof(Char) * string->length + 1);
    return string;
}

StringRef StringCreateCopy(AllocatorRef allocator, StringRef string) {
    StringRef copy = AllocatorAllocate(allocator, sizeof(struct _String));
    assert(copy);
    copy->allocator = allocator;
    copy->length    = string->length;
    copy->memory    = AllocatorAllocate(allocator, sizeof(Char) * string->length + 1);
    assert(copy->memory);
    memcpy(copy->memory, string->memory, sizeof(Char) * string->length + 1);
    return copy;
}

StringRef StringCreateEmpty(AllocatorRef allocator) {
    StringRef string = AllocatorAllocate(allocator, sizeof(struct _String));
    assert(string);
    string->allocator = allocator;
    string->length    = 0;
    string->memory    = AllocatorAllocate(allocator, sizeof(Char) * 8);
    assert(string->memory);
    memset(string->memory, 0, sizeof(Char) * 8);
    return string;
}

void StringDestroy(StringRef string) {
    AllocatorDeallocate(string->allocator, string->memory);
    AllocatorDeallocate(string->allocator, string);
}

Index StringGetLength(StringRef string) {
    return string->length;
}

const Char *StringGetCharacters(StringRef string) {
    return string->memory;
}

void StringAppend(StringRef string, const Char *rawString) {
    Index length = strlen(rawString);
    if (length > 0) {
        Index newCapacity = sizeof(Char) * string->length + length + 1;
        Char *newMemory   = AllocatorReallocate(string->allocator, string->memory, newCapacity);
        assert(newMemory);
        memcpy(newMemory + sizeof(Char) * string->length, rawString, length);
        memset(newMemory + sizeof(Char) * (string->length + length), 0, sizeof(Char));
        string->length += length;
        string->memory = newMemory;
    }
}

void StringAppendString(StringRef string, StringRef other) {
    if (other->length > 0) {
        Index newCapacity = sizeof(Char) * string->length + other->length + 1;
        Char *newMemory   = AllocatorReallocate(string->allocator, string->memory, newCapacity);
        assert(newMemory);
        memcpy(newMemory + sizeof(Char) * string->length, other->memory, other->length);
        memset(newMemory + sizeof(Char) * (string->length + other->length), 0, sizeof(Char));
        string->length += other->length;
        string->memory = newMemory;
    }
}

bool StringIsEqual(StringRef lhs, StringRef rhs) {
    if (lhs->length != rhs->length) {
        return false;
    }

    if (lhs->length > 0) {
        return memcmp(lhs->memory, rhs->memory, sizeof(Char) * lhs->length);
    }

    return true;
}
