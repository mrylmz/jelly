#ifndef __JELLY_NAMERESOLUTION__
#define __JELLY_NAMERESOLUTION__

#include <JellyCore/ASTContext.h>
#include <JellyCore/ASTNodes.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

void PerformNameResolution(ASTContextRef context, ASTModuleDeclarationRef module);

JELLY_EXTERN_C_END

#endif
