#ifndef __JELLY_TEMPALLOCATOR__
#define __JELLY_TEMPALLOCATOR__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

AllocatorRef TempAllocatorCreate(AllocatorRef allocator);

JELLY_EXTERN_C_END

#endif
