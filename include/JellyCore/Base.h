#ifndef __JELLY_BASE__
#define __JELLY_BASE__

#include <assert.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#define JELLY_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#define JELLY_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
#else
#define JELLY_GCC_VERSION 0
#endif

#ifdef __GNUC__
#define JELLY_ATTRIBUTE_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define JELLY_ATTRIBUTE_NORETURN __declspec(noreturn)
#else
#define JELLY_ATTRIBUTE_NORETURN
#endif

JELLY_ATTRIBUTE_NORETURN void __jelly_unreachable(const Char *message, const Char *file, Index line);

#ifndef JELLY_UNREACHABLE
#define JELLY_UNREACHABLE(__MESSAGE__) __jelly_unreachable(__MESSAGE__, __FILE__, __LINE__)
#endif

#ifndef JELLY_PRINTFLIKE
#define JELLY_PRINTFLIKE(__FORMAT_INDEX__, __VARARG_INDEX__) __attribute__((__format__(__printf__, __FORMAT_INDEX__, __VARARG_INDEX__)))
#endif

#ifndef MIN
#define MIN(__X__, __Y__) __X__ <= __Y__ ? __X__ : __Y__
#endif

#ifndef MAX
#define MAX(__X__, __Y__) __X__ >= __Y__ ? __X__ : __Y__
#endif

#ifndef CONCAT
#define CONCAT(__LHS__, __RHS__) __LHS__##__RHS__
#endif

#ifndef UNIQUE
#define UNIQUE(__PREFIX__) CONCAT(__PREFIX__, __COUNTER__)
#endif

#endif
