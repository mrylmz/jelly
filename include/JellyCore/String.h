#ifndef __JELLY_STRING__
#define __JELLY_STRING__

#include <JellyCore/Base.h>
#include <JellyCore/Allocator.h>

JELLY_EXTERN_C_BEGIN

typedef struct _String* StringRef;

StringRef StringCreate(AllocatorRef allocator, const Char *rawString);

StringRef StringCreateCopy(AllocatorRef allocator, StringRef string);

StringRef StringCreateEmpty(AllocatorRef allocator);

void StringDestroy(StringRef string);

Index StringGetLength(StringRef string);

const Char *StringGetCharacters(StringRef string);

void StringAppend(StringRef string, const Char *rawString);

void StringAppendString(StringRef string, StringRef other);

bool StringIsEqual(StringRef lhs, StringRef rhs);

JELLY_EXTERN_C_END

#endif