#ifndef UNARY_OPERATOR
#define UNARY_OPERATOR(SYMBOL, ARGUMENT_TYPE, RESULT_TYPE, FOREIGN_NAME)
#endif

// TODO: Add all possible integer and float type overloads

UNARY_OPERATOR("!", "Bool", "Bool", "JellyLogicalNot")
UNARY_OPERATOR("~", "Int", "Int", "JellyBitwiseNot_Int64")
UNARY_OPERATOR("+", "Int", "Int", "JellyUnaryPlus_Int64")
UNARY_OPERATOR("-", "Int", "Int", "JellyUnaryMinus_Int64")
