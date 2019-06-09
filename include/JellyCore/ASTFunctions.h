#ifndef __JELLY_ASTFUNCTIONS__
#define __JELLY_ASTFUNCTIONS__

#include <JellyCore/ASTContext.h>
#include <JellyCore/ASTNodes.h>

JELLY_EXTERN_C_BEGIN

ASTOperatorPrecedence ASTGetBinaryOperatorPrecedence(ASTBinaryOperator binary);

ASTOperatorPrecedence ASTGetOperatorPrecedenceBefore(ASTOperatorPrecedence precedence);

ASTOperatorAssociativity ASTGetBinaryOperatorAssociativity(ASTBinaryOperator binary);

JELLY_EXTERN_C_END

#endif
