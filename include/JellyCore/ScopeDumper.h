#ifndef __JELLY_SCOPEDUMPER__
#define __JELLY_SCOPEDUMPER__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>
#include <JellyCore/SymbolTable.h>

JELLY_EXTERN_C_BEGIN

typedef struct _ScopeDumper *ScopeDumperRef;

ScopeDumperRef ScopeDumperCreate(AllocatorRef allocator, FILE *target);

void ScopeDumperDestroy(ScopeDumperRef dumper);

void ScopeDumperDump(ScopeDumperRef dumper, ScopeRef scope);

JELLY_EXTERN_C_END

#endif
