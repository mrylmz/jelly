#ifndef __JELLY_ASTMANGLING__
#define __JELLY_ASTMANGLING__

#include <JellyCore/ASTContext.h>
#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

void PerformNameMangling(ASTContextRef context, ASTModuleDeclarationRef module);
void PerformNameManglingForDeclaration(ASTContextRef context, ASTDeclarationRef declaration);

JELLY_EXTERN_C_END

#endif
