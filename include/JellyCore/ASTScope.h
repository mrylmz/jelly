#ifndef __JELLY_ASTSCOPE__
#define __JELLY_ASTSCOPE__

#include <JellyCore/ASTContext.h>
#include <JellyCore/ASTNodes.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN
ASTScopeRef ASTScopeGetNextParentForLookup(ASTScopeRef scope);

void ASTScopeInsertDeclaration(ASTScopeRef scope, ASTDeclarationRef declaration);

ASTDeclarationRef ASTScopeLookupDeclaration(ASTScopeRef scope, StringRef name, const Char *virtualEndOfScope);

void ASTScopeDump(ASTScopeRef scope, FILE *target);

JELLY_EXTERN_C_END

#endif
