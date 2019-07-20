#ifndef __JELLY_STRING__
#define __JELLY_STRING__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _String *StringRef;

StringRef StringCreate(AllocatorRef allocator, const Char *rawString);

StringRef StringCreateRange(AllocatorRef allocator, const Char *start, const Char *end);

StringRef StringCreateCopy(AllocatorRef allocator, StringRef string);

/// Searches the `string` for `character` and returns a copy of the `string` beginning at the last occurence of `character`.
StringRef StringCreateCopyFromLastOccurenceOf(AllocatorRef allocator, StringRef string, Char character);

/// Searches the `string` for `character` and returns a copy of the `string` up to the last occurence of `character`
/// or if the `character` is not found then an empty string is returned.
StringRef StringCreateCopyUntilLastOccurenceOf(AllocatorRef allocator, StringRef string, Char character);

StringRef StringCreateEmpty(AllocatorRef allocator);

StringRef StringCreateFromFile(AllocatorRef allocator, const Char *filePath);

void StringDestroy(StringRef string);

Index StringGetLength(StringRef string);

Char *StringGetCharacters(StringRef string);

void StringAppend(StringRef string, const Char *rawString);

void StringAppendString(StringRef string, StringRef other);

void StringAppendFormat(StringRef string, const Char *format, ...) JELLY_PRINTFLIKE(2, 3);

Bool StringIsEqual(StringRef lhs, StringRef rhs);

Bool StringIsEqualToCString(StringRef lhs, const Char* rawString);

JELLY_EXTERN_C_END

#endif
