#ifndef __JELLY_BASE__
#define __JELLY_BASE__

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef bool Bool;
typedef char Char;
typedef int8_t Int8;
typedef int16_t Int16;
typedef int32_t Int32;
typedef int64_t Int64;
typedef Int64 Int;
typedef uint8_t UInt8;
typedef uint16_t UInt16;
typedef uint32_t UInt32;
typedef uint64_t UInt64;
typedef UInt64 UInt;
typedef float Float32;
typedef double Float64;
typedef Float64 Float;
typedef size_t Index;

#ifdef __cplusplus
#define JELLY_EXTERN_C_BEGIN extern "C" {
#define JELLY_EXTERN_C_END }
#else
#define JELLY_EXTERN_C_BEGIN
#define JELLY_EXTERN_C_END
#endif

// TODO: Use cross compiler supported builtin unreachable to disable warnings!
#ifndef JELLY_UNREACHABLE
#define JELLY_UNREACHABLE(__MESSAGE__) assert(0 && __MESSAGE__)
#endif

#ifndef MIN
#define MIN(__X__, __Y__) __X__ <= __Y__ ? __X__ : __Y__
#endif

#ifndef MAX
#define MAX(__X__, __Y__) __X__ >= __Y__ ? __X__ : __Y__
#endif

#endif
