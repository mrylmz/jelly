#include "JellyCore/ASTFunctions.h"

ASTOperatorPrecedence ASTGetBinaryOperatorPrecedence(ASTBinaryOperator binary) {
    switch (binary) {
    case ASTBinaryOperatorBitwiseLeftShift:
    case ASTBinaryOperatorBitwiseRightShift:
        return 900;

    case ASTBinaryOperatorMultiply:
    case ASTBinaryOperatorDivide:
    case ASTBinaryOperatorReminder:
    case ASTBinaryOperatorBitwiseAnd:
        return 800;

    case ASTBinaryOperatorAdd:
    case ASTBinaryOperatorSubtract:
    case ASTBinaryOperatorBitwiseOr:
    case ASTBinaryOperatorBitwiseXor:
        return 700;

    case ASTBinaryOperatorTypeCheck:
    case ASTBinaryOperatorTypeCast:
        return 500;

    case ASTBinaryOperatorLessThan:
    case ASTBinaryOperatorLessThanEqual:
    case ASTBinaryOperatorGreaterThan:
    case ASTBinaryOperatorGreaterThanEqual:
    case ASTBinaryOperatorEqual:
    case ASTBinaryOperatorNotEqual:
        return 400;

    case ASTBinaryOperatorLogicalAnd:
        return 300;

    case ASTBinaryOperatorLogicalOr:
        return 200;

    case ASTBinaryOperatorAssign:
    case ASTBinaryOperatorMultiplyAssign:
    case ASTBinaryOperatorDivideAssign:
    case ASTBinaryOperatorReminderAssign:
    case ASTBinaryOperatorAddAssign:
    case ASTBinaryOperatorSubtractAssign:
    case ASTBinaryOperatorBitwiseLeftShiftAssign:
    case ASTBinaryOperatorBitwiseRightShiftAssign:
    case ASTBinaryOperatorBitwiseAndAssign:
    case ASTBinaryOperatorBitwiseOrAssign:
    case ASTBinaryOperatorBitwiseXorAssign:
        return 100;

    default:
        return 250;
    }
}

ASTOperatorPrecedence ASTGetOperatorPrecedenceBefore(ASTOperatorPrecedence precedence) {
    if (precedence > 900) {
        return 900;
    } else if (precedence > 800) {
        return 800;
    } else if (precedence > 700) {
        return 700;
    } else if (precedence > 500) {
        return 500;
    } else if (precedence > 400) {
        return 400;
    } else if (precedence > 300) {
        return 300;
    } else if (precedence > 250) {
        return 250;
    } else if (precedence > 200) {
        return 200;
    } else if (precedence > 100) {
        return 100;
    } else {
        return 0;
    }
}

ASTOperatorAssociativity ASTGetBinaryOperatorAssociativity(ASTBinaryOperator binary) {
    switch (binary) {
    case ASTBinaryOperatorBitwiseLeftShift:
    case ASTBinaryOperatorBitwiseRightShift:
        return ASTOperatorAssociativityNone;

    case ASTBinaryOperatorMultiply:
    case ASTBinaryOperatorDivide:
    case ASTBinaryOperatorReminder:
    case ASTBinaryOperatorBitwiseAnd:
    case ASTBinaryOperatorAdd:
    case ASTBinaryOperatorSubtract:
    case ASTBinaryOperatorBitwiseOr:
    case ASTBinaryOperatorBitwiseXor:
    case ASTBinaryOperatorTypeCheck:
    case ASTBinaryOperatorTypeCast:
        return ASTOperatorAssociativityLeft;

    case ASTBinaryOperatorLessThan:
    case ASTBinaryOperatorLessThanEqual:
    case ASTBinaryOperatorGreaterThan:
    case ASTBinaryOperatorGreaterThanEqual:
    case ASTBinaryOperatorEqual:
    case ASTBinaryOperatorNotEqual:
        return ASTOperatorAssociativityNone;

    case ASTBinaryOperatorLogicalAnd:
    case ASTBinaryOperatorLogicalOr:
        return ASTOperatorAssociativityLeft;

    case ASTBinaryOperatorAssign:
    case ASTBinaryOperatorMultiplyAssign:
    case ASTBinaryOperatorDivideAssign:
    case ASTBinaryOperatorReminderAssign:
    case ASTBinaryOperatorAddAssign:
    case ASTBinaryOperatorSubtractAssign:
    case ASTBinaryOperatorBitwiseLeftShiftAssign:
    case ASTBinaryOperatorBitwiseRightShiftAssign:
    case ASTBinaryOperatorBitwiseAndAssign:
    case ASTBinaryOperatorBitwiseOrAssign:
    case ASTBinaryOperatorBitwiseXorAssign:
        return ASTOperatorAssociativityRight;

    default:
        return ASTOperatorAssociativityNone;
    }
}
