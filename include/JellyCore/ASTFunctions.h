#ifndef __JELLY_ASTFUNCTIONS__
#define __JELLY_ASTFUNCTIONS__

#include <JellyCore/ASTContext.h>
#include <JellyCore/ASTNodes.h>

JELLY_EXTERN_C_BEGIN

ASTOperatorPrecedence ASTGetBinaryOperatorPrecedence(ASTBinaryOperator binary);

ASTOperatorPrecedence ASTGetPostfixOperatorPrecedence(ASTPostfixOperator op);

ASTOperatorPrecedence ASTGetOperatorPrecedenceBefore(ASTOperatorPrecedence precedence);

ASTOperatorAssociativity ASTGetBinaryOperatorAssociativity(ASTBinaryOperator binary);

Bool ASTBinaryOperatorIsAssignment(ASTBinaryOperator binary);

StringRef ASTGetPrefixOperatorName(AllocatorRef allocator, ASTUnaryOperator op);

StringRef ASTGetInfixOperatorName(AllocatorRef allocator, ASTBinaryOperator op);

Bool ASTTypeIsEqual(ASTTypeRef lhs, ASTTypeRef rhs);

Bool ASTTypeIsInteger(ASTTypeRef type);

Bool ASTTypeIsVoid(ASTTypeRef type);

JELLY_EXTERN_C_END

#endif
