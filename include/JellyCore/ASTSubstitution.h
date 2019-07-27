#ifndef __JELLY_ASTSUBSTITUTION__
#define __JELLY_ASTSUBSTITUTION__

#include <JellyCore/ASTContext.h>
#include <JellyCore/ASTNodes.h>
#include <JellyCore/Base.h>

JELLY_EXTERN_C_BEGIN

typedef ASTNodeRef (*ASTTransform)(ASTContextRef context, ASTNodeRef node);

void ASTPerformSubstitution(ASTContextRef context, ASTTag tag, ASTTransform transform);
void ASTApplySubstitution(ASTContextRef context, ASTModuleDeclarationRef module);

ASTNodeRef ASTUnaryExpressionUnification(ASTContextRef context, ASTNodeRef node);
ASTNodeRef ASTBinaryExpressionUnification(ASTContextRef context, ASTNodeRef node);

JELLY_EXTERN_C_END

#endif
