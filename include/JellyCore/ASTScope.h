#ifndef __JELLY_ASTSCOPE__
#define __JELLY_ASTSCOPE__

#include <JellyCore/ASTContext.h>
#include <JellyCore/ASTNodes.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

ASTScopeRef ASTScopeGetNextParentForLookup(ASTScopeRef scope);

void ASTScopeInsertDeclaration(ASTScopeRef scope, ASTDeclarationRef declaration);

ASTDeclarationRef ASTScopeLookupDeclarationByName(ASTScopeRef scope, StringRef name);

ASTDeclarationRef ASTScopeLookupDeclarationByNameOrMatchingFunctionSignature(ASTScopeRef scope, StringRef name, ASTArrayRef parameters,
                                                                             ASTTypeRef resultType);

ASTFunctionDeclarationRef ASTScopeLookupFunctionDeclarationByNameAndArguments(ASTScopeRef scope, StringRef name, ASTArrayRef arguments);

ASTDeclarationRef ASTScopeLookupDeclarationInHierarchyByName(ASTScopeRef scope, StringRef name);

ASTDeclarationRef ASTScopeLookupDeclarationInHierarchyByNameOrMatchingFunctionSignature(ASTScopeRef scope, StringRef name,
                                                                                        ASTArrayRef parameters, ASTTypeRef resultType);

ASTFunctionDeclarationRef ASTScopeLookupFunctionDeclarationInHierarchyByNameAndParameters(ASTScopeRef scope, StringRef name,
                                                                                          ASTArrayRef parameters);

void ASTScopeDump(ASTScopeRef scope, FILE *target);

JELLY_EXTERN_C_END

#endif
