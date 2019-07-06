#ifndef __JELLY_TYPER__
#define __JELLY_TYPER__

#include <JellyCore/ASTContext.h>
#include <JellyCore/ASTNodes.h>
#include <JellyCore/ASTScope.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _Typer *TyperRef;

TyperRef TyperCreate(AllocatorRef allocator);

void TyperDestroy(TyperRef typer);

void TyperType(TyperRef typer, ASTContextRef context, ASTNodeRef node);

JELLY_EXTERN_C_END

#endif
