#ifndef __JELLY_UNARYFUNCTIONS__
#define __JELLY_UNARYFUNCTIONS__

#include <JellyRuntimeSupport/Base.h>

JELLY_EXTERN_C_BEGIN

Bool JellyLogicalNot(Bool);
Int64 JellyBitwiseNot_Int64(Int64);
Int64 JellyUnaryPlus_Int64(Int64);
Int64 JellyUnaryMinus_Int64(Int64);

JELLY_EXTERN_C_END

#endif
