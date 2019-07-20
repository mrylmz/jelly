#ifndef __JELLY_LDLINKER__
#define __JELLY_LDLINKER__

#include <JellyCore/Allocator.h>
#include <JellyCore/Array.h>
#include <JellyCore/Base.h>
#include <JellyCore/String.h>

JELLY_EXTERN_C_BEGIN

enum _LDLinkerTargetType {
    LDLinkerTargetTypeExecutable,
    LDLinkerTargetTypeDylib,
    LDLinkerTargetTypeBundle,
    LDLinkerTargetTypeStatic,
};
typedef enum _LDLinkerTargetType LDLinkerTargetType;

void LDLinkerLink(AllocatorRef allocator, ArrayRef objectFiles, StringRef targetPath, LDLinkerTargetType targetType,
                  StringRef architecture);

JELLY_EXTERN_C_END

#endif
