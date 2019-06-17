#ifndef __JELLY_ASTDUMPER__
#define __JELLY_ASTDUMPER__

#include <JellyCore/ASTNodes.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _ASTDumper *ASTDumperRef;

ASTDumperRef ASTDumperCreate(AllocatorRef allocator, FILE *target);

void ASTDumperDestroy(ASTDumperRef dumper);

void ASTDumperDump(ASTDumperRef dumper, ASTNodeRef node);

JELLY_EXTERN_C_END

#endif
