#ifndef __JELLY_NAMERESOLVER__
#define __JELLY_NAMERESOLVER__

#include <JellyCore/ASTContext.h>
#include <JellyCore/ASTNodes.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef struct _NameResolver *NameResolverRef;

NameResolverRef NameResolverCreate(AllocatorRef allocator);

void NameResolverDestroy(NameResolverRef resolver);

void NameResolverResolve(NameResolverRef resolver, ASTContextRef context, ASTNodeRef node);

JELLY_EXTERN_C_END

#endif
