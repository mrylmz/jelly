#ifndef __JELLY_BINARYFUNCTIONS__
#define __JELLY_BINARYFUNCTIONS__

#include <JellyRuntimeSupport/Base.h>

JELLY_EXTERN_C_BEGIN

Int JellyBitwiseLeftShift_Int64(Int, Int);
Int JellyBitwiseRightShift_Int64(Int, Int);
Int JellyMultiply_Int64(Int, Int);
Int JellyDivide_Int64(Int, Int);
Int JellyReminder_Int64(Int, Int);
Int JellyBitwiseAnd_Int64(Int, Int);
Int JellyAdd_Int64(Int, Int);
Int JellySubtract_Int64(Int, Int);
Int JellyBitwiseOr_Int64(Int, Int);
Int JellyBitwiseXor_Int64(Int, Int);
Bool JellyLessThan_Int64(Int, Int);
Bool JellyLessThanEqual_Int64(Int, Int);
Bool JellyGreaterThen_Int64(Int, Int);
Bool JellyGreaterThanEqual_Int64(Int, Int);
Bool JellyEqual_Int64(Int, Int);
Bool JellyNotEqual_Int64(Int, Int);
Bool JellyLogicalAnd(Bool, Bool);
Bool JellyLogicalOr(Bool, Bool);

JELLY_EXTERN_C_END

#endif
