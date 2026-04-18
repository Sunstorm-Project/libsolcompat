/*
 * sys/int_types.h — Fixed integer typedefs for Solaris 7
 *
 * Replaces the Solaris 7 sys/int_types.h entirely. Two reasons:
 *   1. Solaris 7 typedefs int8_t as plain 'char', which conflicts with
 *      C99 stdint.h (signed char). In C++ this causes template
 *      specialization mismatches.
 *   2. Solaris 7 guards int64_t/uint64_t behind __STRICT_ANSI__
 *      checks that exclude -std=c99/-std=c11 compiles.
 *
 * By replacing the file with a clean set of unconditional typedefs,
 * GCC's fixincludes copies a consistent version into include-fixed/
 * and <stdint.h> gets the correct types.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */
#ifndef _SYS_INT_TYPES_H
#define _SYS_INT_TYPES_H

#include <sys/isa_defs.h>
#include <sys/feature_tests.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exact-width integer types — signed char (not plain char) for int8_t */
typedef signed char         int8_t;
typedef short               int16_t;
typedef int                 int32_t;
typedef long long           int64_t;

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

/* C99 int_least<N>_t — piggyback on the exact-width typedefs above.
 * C99 only requires these to be AT LEAST the given width, so aliasing to
 * the exact-width types is spec-compliant and avoids any independent
 * width claim.  Each is guarded so downstream headers (or a future
 * overlay) can pre-define without conflict.
 *
 * int_fast<N>_t is deliberately NOT declared here — the sysroot-overlay
 * <stdint.h> unconditionally typedefs those, and declaring them here
 * would create a second authority and trigger "conflicting declaration"
 * errors in packages that include both headers. */
#ifndef _INT_LEAST8_T
#define _INT_LEAST8_T
/* 'char' — NOT 'signed char' — because GCC's <stdint-gcc.h> declares
 * int_least8_t as __INT_LEAST8_TYPE__, which the sparc-sun-solaris2.7
 * backend sets to plain 'char'.  Kernel-mode compiles pull stdint-gcc.h
 * after this header; types must match exactly.  See override/stdint.h
 * for the full rationale. */
typedef char int_least8_t;
#endif
#ifndef _INT_LEAST16_T
#define _INT_LEAST16_T
typedef int16_t  int_least16_t;
#endif
#ifndef _INT_LEAST32_T
#define _INT_LEAST32_T
typedef int32_t  int_least32_t;
#endif
#ifndef _INT_LEAST64_T
#define _INT_LEAST64_T
typedef int64_t  int_least64_t;
#endif
#ifndef _UINT_LEAST8_T
#define _UINT_LEAST8_T
typedef uint8_t  uint_least8_t;
#endif
#ifndef _UINT_LEAST16_T
#define _UINT_LEAST16_T
typedef uint16_t uint_least16_t;
#endif
#ifndef _UINT_LEAST32_T
#define _UINT_LEAST32_T
typedef uint32_t uint_least32_t;
#endif
#ifndef _UINT_LEAST64_T
#define _UINT_LEAST64_T
typedef uint64_t uint_least64_t;
#endif

/* Maximum-width integer types */
typedef long long           intmax_t;
typedef unsigned long long  uintmax_t;

/* Pointer-width integer types (ILP32 on Solaris 7 SPARC) */
typedef int                 intptr_t;
typedef unsigned int        uintptr_t;

/* Also define guard macros that other Solaris headers check, so
 * subsequent includes don't redefine these types. */
#define _INT8_T
#define _INT16_T
#define _INT32_T
#define _INT64_T
#define _UINT8_T
#define _UINT16_T
#define _UINT32_T
#define _UINT64_T
#define _INTPTR_T
#define _UINTPTR_T
#define __int8_t_defined
#define __uint32_t_defined

#ifdef __cplusplus
}
#endif

#endif /* _SYS_INT_TYPES_H */
