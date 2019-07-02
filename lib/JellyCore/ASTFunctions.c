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

ASTOperatorPrecedence ASTGetPostfixOperatorPrecedence(ASTPostfixOperator op) {
    switch (op) {
    case ASTPostfixOperatorUnknown:
        return 250;

    default:
        return 1000;
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

StringRef ASTGetPrefixOperatorName(AllocatorRef allocator, ASTUnaryOperator op) {
    switch (op) {
    case ASTUnaryOperatorLogicalNot:
        return StringCreate(allocator, "!");

    case ASTUnaryOperatorBitwiseNot:
        return StringCreate(allocator, "~");

    case ASTUnaryOperatorUnaryPlus:
        return StringCreate(allocator, "+");

    case ASTUnaryOperatorUnaryMinus:
        return StringCreate(allocator, "-");

    default:
        JELLY_UNREACHABLE("Unknown value given for prefix operator!");
        break;
    }
}

StringRef ASTGetInfixOperatorName(AllocatorRef allocator, ASTBinaryOperator op) {
    switch (op) {
    case ASTBinaryOperatorBitwiseLeftShift:
        return StringCreate(allocator, "<<");
    case ASTBinaryOperatorBitwiseRightShift:
        return StringCreate(allocator, ">>");
    case ASTBinaryOperatorMultiply:
        return StringCreate(allocator, "*");
    case ASTBinaryOperatorDivide:
        return StringCreate(allocator, "/");
    case ASTBinaryOperatorReminder:
        return StringCreate(allocator, "%");
    case ASTBinaryOperatorBitwiseAnd:
        return StringCreate(allocator, "&");
    case ASTBinaryOperatorAdd:
        return StringCreate(allocator, "+");
    case ASTBinaryOperatorSubtract:
        return StringCreate(allocator, "-");
    case ASTBinaryOperatorBitwiseOr:
        return StringCreate(allocator, "|");
    case ASTBinaryOperatorBitwiseXor:
        return StringCreate(allocator, "^");
    case ASTBinaryOperatorTypeCheck:
        return StringCreate(allocator, "is");
    case ASTBinaryOperatorTypeCast:
        return StringCreate(allocator, "as");
    case ASTBinaryOperatorLessThan:
        return StringCreate(allocator, "<");
    case ASTBinaryOperatorLessThanEqual:
        return StringCreate(allocator, "<=");
    case ASTBinaryOperatorGreaterThan:
        return StringCreate(allocator, ">");
    case ASTBinaryOperatorGreaterThanEqual:
        return StringCreate(allocator, ">=");
    case ASTBinaryOperatorEqual:
        return StringCreate(allocator, "==");
    case ASTBinaryOperatorNotEqual:
        return StringCreate(allocator, "!=");
    case ASTBinaryOperatorLogicalAnd:
        return StringCreate(allocator, "&&");
    case ASTBinaryOperatorLogicalOr:
        return StringCreate(allocator, "||");
    case ASTBinaryOperatorAssign:
        return StringCreate(allocator, "=");
    case ASTBinaryOperatorMultiplyAssign:
        return StringCreate(allocator, "*=");
    case ASTBinaryOperatorDivideAssign:
        return StringCreate(allocator, "/=");
    case ASTBinaryOperatorReminderAssign:
        return StringCreate(allocator, "%=");
    case ASTBinaryOperatorAddAssign:
        return StringCreate(allocator, "+=");
    case ASTBinaryOperatorSubtractAssign:
        return StringCreate(allocator, "-=");
    case ASTBinaryOperatorBitwiseLeftShiftAssign:
        return StringCreate(allocator, "<<=");
    case ASTBinaryOperatorBitwiseRightShiftAssign:
        return StringCreate(allocator, ">>=");
    case ASTBinaryOperatorBitwiseAndAssign:
        return StringCreate(allocator, "&=");
    case ASTBinaryOperatorBitwiseOrAssign:
        return StringCreate(allocator, "|=");
    case ASTBinaryOperatorBitwiseXorAssign:
        return StringCreate(allocator, "^=");

    default:
        JELLY_UNREACHABLE("Unknown value given for infix operator!");
        break;
    }
}
