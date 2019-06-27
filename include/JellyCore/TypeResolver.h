#ifndef __JELLY_TYPERESOLVER__
#define __JELLY_TYPERESOLVER__

#include <JellyCore/ASTContext.h>
#include <JellyCore/ASTNodes.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _TypeResolver *TypeResolverRef;

TypeResolverRef TypeResolverCreate(AllocatorRef allocator);

void TypeResolverDestroy(TypeResolverRef resolver);

void TypeResolverResolve(TypeResolverRef resolver, ASTContextRef context, ASTNodeRef node);

JELLY_EXTERN_C_END

#endif
