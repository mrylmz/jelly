#ifndef UNARY_OPERATOR
#define UNARY_OPERATOR(SYMBOL, ARGUMENT_TYPE, RESULT_TYPE, FOREIGN_NAME)
#endif

// TODO: Add all possible integer and float type overloads

UNARY_OPERATOR("!", "Bool", "Bool", "JellyLogicalNot")
UNARY_OPERATOR("~", "Int", "Int", "JellyBitwiseNot_Int64")
UNARY_OPERATOR("+", "Int", "Int", "JellyUnaryPlus_Int64")
UNARY_OPERATOR("-", "Int", "Int", "JellyUnaryMinus_Int64")

#ifndef BINARY_OPERATOR
#define BINARY_OPERATOR(SYMBOL, ARGUMENT_TYPE1, ARGUMENT_TYPE2, RESULT_TYPE, FOREIGN_NAME)
#endif

BINARY_OPERATOR("<<", "Int", "Int", "Int", "JellyBitwiseLeftShift_Int64")
BINARY_OPERATOR(">>", "Int", "Int", "Int", "JellyBitwiseRightShift_Int64")
BINARY_OPERATOR("*", "Int", "Int", "Int", "JellyMultiply_Int64")
BINARY_OPERATOR("/", "Int", "Int", "Int", "JellyDivide_Int64")
BINARY_OPERATOR("%", "Int", "Int", "Int", "JellyReminder_Int64")
BINARY_OPERATOR("&", "Int", "Int", "Int", "JellyBitwiseAnd_Int64")
BINARY_OPERATOR("+", "Int", "Int", "Int", "JellyAdd_Int64")
BINARY_OPERATOR("-", "Int", "Int", "Int", "JellySubtract_Int64")
BINARY_OPERATOR("|", "Int", "Int", "Int", "JellyBitwiseOr_Int64")
BINARY_OPERATOR("^", "Int", "Int", "Int", "JellyBitwiseXor_Int64")
BINARY_OPERATOR("<", "Int", "Int", "Bool", "JellyLessThan_Int64")
BINARY_OPERATOR("<=", "Int", "Int", "Bool", "JellyLessThanEqual_Int64")
BINARY_OPERATOR(">", "Int", "Int", "Bool", "JellyGreaterThen_Int64")
BINARY_OPERATOR(">=", "Int", "Int", "Bool", "JellyGreaterThanEqual_Int64")
BINARY_OPERATOR("==", "Int", "Int", "Bool", "JellyEqual_Int64")
BINARY_OPERATOR("!=", "Int", "Int", "Bool", "JellyNotEqual_Int64")

BINARY_OPERATOR("&&", "Bool", "Bool", "Bool", "JellyLogicalAnd")
BINARY_OPERATOR("||", "Bool", "Bool", "Bool", "JellyLogicalOr")

#undef UNARY_OPERATOR
#undef BINARY_OPERATOR
