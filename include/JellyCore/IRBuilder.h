#ifndef __JELLY_IRBUILDER__
#define __JELLY_IRBUILDER__

#include <JellyCore/ASTNodes.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _IRBuilder *IRBuilderRef;

IRBuilderRef IRBuilderCreate(AllocatorRef allocator);

void IRBuilderDestroy(IRBuilderRef builder);

void IRBuilderBuild(IRBuilderRef builder, ASTModuleDeclarationRef module);

JELLY_EXTERN_C_END

#endif
