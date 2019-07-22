#ifndef __JELLY_IRBUILDER__
#define __JELLY_IRBUILDER__

#include <JellyCore/ASTNodes.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _IRBuilder *IRBuilderRef;
typedef struct _IRModule *IRModuleRef;

IRBuilderRef IRBuilderCreate(AllocatorRef allocator, ASTContextRef context, StringRef buildDirectory);

void IRBuilderDestroy(IRBuilderRef builder);

IRModuleRef IRBuilderBuild(IRBuilderRef builder, ASTModuleDeclarationRef module);

void IRBuilderDumpModule(IRBuilderRef builder, IRModuleRef module, FILE *target);

void IRBuilderVerifyModule(IRBuilderRef builder, IRModuleRef module);

void IRBuilderEmitObjectFile(IRBuilderRef builder, IRModuleRef module, StringRef fileName);

JELLY_EXTERN_C_END

#endif
