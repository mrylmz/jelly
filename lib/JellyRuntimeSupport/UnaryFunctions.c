#include "JellyRuntimeSupport/UnaryFunctions.h"

Bool JellyLogicalNot(Bool value) {
    if (value) {
        return false;
    } else {
        return true;
    }
}

Int64 JellyBitwiseNot_Int64(Int64 value) {
    return ~value;
}

Int64 JellyUnaryPlus_Int64(Int64 value) {
    return +value;
}

Int64 JellyUnaryMinus_Int64(Int64 value) {
    return -value;
}
