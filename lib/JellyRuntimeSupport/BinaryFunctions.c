#include "JellyRuntimeSupport/BinaryFunctions.h"

Int JellyBitwiseLeftShift_Int64(Int lhs, Int rhs) {
    return lhs << rhs;
}

Int JellyBitwiseRightShift_Int64(Int lhs, Int rhs) {
    return lhs >> rhs;
}

Int JellyMultiply_Int64(Int lhs, Int rhs) {
    return lhs * rhs;
}

Int JellyDivide_Int64(Int lhs, Int rhs) {
    return lhs / rhs;
}

Int JellyReminder_Int64(Int lhs, Int rhs) {
    return lhs % rhs;
}

Int JellyBitwiseAnd_Int64(Int lhs, Int rhs) {
    return lhs & rhs;
}

Int JellyAdd_Int64(Int lhs, Int rhs) {
    return lhs + rhs;
}

Int JellySubtract_Int64(Int lhs, Int rhs) {
    return lhs - rhs;
}

Int JellyBitwiseOr_Int64(Int lhs, Int rhs) {
    return lhs | rhs;
}

Int JellyBitwiseXor_Int64(Int lhs, Int rhs) {
    return lhs ^ rhs;
}

Bool JellyLessThan_Int64(Int lhs, Int rhs) {
    return lhs < rhs;
}

Bool JellyLessThanEqual_Int64(Int lhs, Int rhs) {
    return lhs <= rhs;
}

Bool JellyGreaterThen_Int64(Int lhs, Int rhs) {
    return lhs > rhs;
}

Bool JellyGreaterThanEqual_Int64(Int lhs, Int rhs) {
    return lhs >= rhs;
}

Bool JellyEqual_Int64(Int lhs, Int rhs) {
    return lhs == rhs;
}

Bool JellyNotEqual_Int64(Int lhs, Int rhs) {
    return lhs != rhs;
}

Bool JellyLogicalAnd(Bool lhs, Bool rhs) {
    return lhs && rhs;
}

Bool JellyLogicalOr(Bool lhs, Bool rhs) {
    return lhs || rhs;
}
