#include "JellyCore/String.h"

// TODO: Allow dynamic buffer sizes for string formatting, this buffer is only a temporary solution and unlikely to cause memory issues for
// now because there won't be any known string which more than 65535 characters...
static Char _kStringFormatBuffer[65535] = {};

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

StringRef StringCreateRange(AllocatorRef allocator, const Char *start, const Char *end) {
    Index length     = end - start;
    StringRef string = AllocatorAllocate(allocator, sizeof(struct _String));
    assert(string);
    string->allocator = allocator;
    string->length    = length;
    string->memory    = AllocatorAllocate(allocator, sizeof(Char) * length + 1);
    assert(string->memory);
    memcpy(string->memory, start, sizeof(Char) * length);
    string->memory[sizeof(Char) * length] = '\0';
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

StringRef StringCreateCopyFromLastOccurenceOf(AllocatorRef allocator, StringRef string, Char character) {
    for (Index index = 0; index < StringGetLength(string); index++) {
        const Char *current = &string->memory[string->length - index - 1];
        if (*current == character) {
            return StringCreateRange(allocator, current + 1, &string->memory[string->length]);
        }
    }

    return StringCreateCopy(allocator, string);
}

StringRef StringCreateCopyUntilLastOccurenceOf(AllocatorRef allocator, StringRef string, Char character) {
    for (Index index = 0; index < StringGetLength(string); index++) {
        const Char *current = &string->memory[string->length - index - 1];
        if (*current == character) {
            return StringCreateRange(allocator, string->memory, current);
        }
    }

    return StringCreateEmpty(allocator);
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

StringRef StringCreateFromFile(AllocatorRef allocator, const Char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    Index length = ftell(file);
    fseek(file, 0, SEEK_SET);
    Char *memory = AllocatorAllocate(allocator, sizeof(Char) * length + 1);
    assert(memory);
    fread(memory, sizeof(Char), length, file);
    memory[length] = 0;
    fclose(file);

    StringRef string = AllocatorAllocate(allocator, sizeof(struct _String));
    assert(string);
    string->allocator = allocator;
    string->length    = length;
    string->memory    = memory;
    return string;
}

void StringDestroy(StringRef string) {
    AllocatorDeallocate(string->allocator, string->memory);
    AllocatorDeallocate(string->allocator, string);
}

Index StringGetLength(StringRef string) {
    return string->length;
}

Char *StringGetCharacters(StringRef string) {
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

void StringAppendFormat(StringRef string, const Char *format, ...) {
    va_list argumentPointer;
    va_start(argumentPointer, format);
    vsprintf(&_kStringFormatBuffer[0], format, argumentPointer);
    va_end(argumentPointer);

    StringAppend(string, &_kStringFormatBuffer[0]);
}

Bool StringIsEqual(StringRef lhs, StringRef rhs) {
    if (lhs->length != rhs->length) {
        return false;
    }

    if (lhs->length > 0) {
        return memcmp(lhs->memory, rhs->memory, sizeof(Char) * lhs->length) == 0;
    }

    return true;
}
