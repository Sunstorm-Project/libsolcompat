/*
 * override/stdint.h — C99 integer types for Solaris 7
 *
 * Solaris 7 doesn't have a native <stdint.h>.  This override header
 * provides one using <sys/int_types.h> which has the base types.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_STDINT_H
#define _SOLCOMPAT_OVERRIDE_STDINT_H

#include <sys/int_types.h>

/* ================================================================
 * Exact-width integer limits
 * ================================================================ */
#define INT8_MIN    (-128)
#define INT8_MAX    127
#define UINT8_MAX   255U
#define INT16_MIN   (-32768)
#define INT16_MAX   32767
#define UINT16_MAX  65535U
#define INT32_MIN   (-2147483647-1)
#define INT32_MAX   2147483647
#define UINT32_MAX  4294967295U
#define INT64_MIN   (-9223372036854775807LL-1LL)
#define INT64_MAX   9223372036854775807LL
#define UINT64_MAX  18446744073709551615ULL

/* ================================================================
 * Minimum-width integer limits
 * ================================================================ */
#define INT_LEAST8_MIN   INT8_MIN
#define INT_LEAST8_MAX   INT8_MAX
#define UINT_LEAST8_MAX  UINT8_MAX
#define INT_LEAST16_MIN  INT16_MIN
#define INT_LEAST16_MAX  INT16_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define INT_LEAST32_MIN  INT32_MIN
#define INT_LEAST32_MAX  INT32_MAX
#define UINT_LEAST32_MAX UINT32_MAX
#define INT_LEAST64_MIN  INT64_MIN
#define INT_LEAST64_MAX  INT64_MAX
#define UINT_LEAST64_MAX UINT64_MAX

/* ================================================================
 * Fastest minimum-width integer limits
 * ================================================================ */
#define INT_FAST8_MIN    INT32_MIN
#define INT_FAST8_MAX    INT32_MAX
#define UINT_FAST8_MAX   UINT32_MAX
#define INT_FAST16_MIN   INT32_MIN
#define INT_FAST16_MAX   INT32_MAX
#define UINT_FAST16_MAX  UINT32_MAX
#define INT_FAST32_MIN   INT32_MIN
#define INT_FAST32_MAX   INT32_MAX
#define UINT_FAST32_MAX  UINT32_MAX
#define INT_FAST64_MIN   INT64_MIN
#define INT_FAST64_MAX   INT64_MAX
#define UINT_FAST64_MAX  UINT64_MAX

/* ================================================================
 * Pointer-width integer limits (ILP32 — SPARC 32-bit)
 * ================================================================ */
#define INTPTR_MIN   INT32_MIN
#define INTPTR_MAX   INT32_MAX
#define UINTPTR_MAX  UINT32_MAX

/* ================================================================
 * Maximum-width integer limits
 * ================================================================ */
#define INTMAX_MIN   INT64_MIN
#define INTMAX_MAX   INT64_MAX
#define UINTMAX_MAX  UINT64_MAX

/* ================================================================
 * Other limits
 * ================================================================ */
#define PTRDIFF_MIN  INT32_MIN
#define PTRDIFF_MAX  INT32_MAX
#define SIZE_MAX     UINT32_MAX
#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX
#define WCHAR_MIN    0
#define WCHAR_MAX    2147483647
#define WINT_MIN     INT32_MIN
#define WINT_MAX     INT32_MAX

/* ================================================================
 * Integer constant macros
 * ================================================================ */
#define INT8_C(x)    (x)
#define UINT8_C(x)   (x ## U)
#define INT16_C(x)   (x)
#define UINT16_C(x)  (x ## U)
#define INT32_C(x)   (x)
#define UINT32_C(x)  (x ## U)
#define INT64_C(x)   (x ## LL)
#define UINT64_C(x)  (x ## ULL)
#define INTMAX_C(x)  (x ## LL)
#define UINTMAX_C(x) (x ## ULL)

/* ================================================================
 * Minimum-width integer types
 * (same as exact-width on SPARC Solaris 7)
 * ================================================================ */
typedef int8_t   int_least8_t;
typedef int16_t  int_least16_t;
typedef int32_t  int_least32_t;
typedef int64_t  int_least64_t;
typedef uint8_t  uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;

/* ================================================================
 * Fastest minimum-width integer types
 * (SPARC favors 32-bit alignment for narrow types)
 * ================================================================ */
typedef int32_t  int_fast8_t;
typedef int32_t  int_fast16_t;
typedef int32_t  int_fast32_t;
typedef int64_t  int_fast64_t;
typedef uint32_t uint_fast8_t;
typedef uint32_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
typedef uint64_t uint_fast64_t;

#endif /* _SOLCOMPAT_OVERRIDE_STDINT_H */
