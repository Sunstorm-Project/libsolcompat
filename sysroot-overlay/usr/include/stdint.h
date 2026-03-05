/*
 * override/stdint.h -- C99 integer types for Solaris 7 SPARC
 *
 * Solaris 7 has no native <stdint.h>.  Its <sys/int_types.h> provides
 * the base types, but guards int64_t/uint64_t behind
 *   #if __STDC__ - 0 == 0
 * which fails under -std=c99/-std=c11 (where __STDC__ == 1).
 *
 * This header defines all types directly so they work in any mode.
 *
 * Part of libsolcompat -- https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_STDINT_H
#define _SOLCOMPAT_OVERRIDE_STDINT_H

/* ================================================================
 * Exact-width integer types -- defined unconditionally
 * ================================================================ */
#ifndef _INT8_T
#define _INT8_T
typedef signed char             int8_t;
#endif
#ifndef _INT16_T
#define _INT16_T
typedef short                   int16_t;
#endif
#ifndef _INT32_T
#define _INT32_T
typedef int                     int32_t;
#endif
#ifndef _INT64_T
#define _INT64_T
__extension__ typedef long long int64_t;
#endif

#ifndef _UINT8_T
#define _UINT8_T
typedef unsigned char           uint8_t;
#endif
#ifndef _UINT16_T
#define _UINT16_T
typedef unsigned short          uint16_t;
#endif
#ifndef _UINT32_T
#define _UINT32_T
typedef unsigned int            uint32_t;
#endif
#ifndef _UINT64_T
#define _UINT64_T
__extension__ typedef unsigned long long uint64_t;
#endif

/* intmax_t / uintmax_t */
#ifndef _INTMAX_T
#define _INTMAX_T
__extension__ typedef long long intmax_t;
#endif
#ifndef _UINTMAX_T
#define _UINTMAX_T
__extension__ typedef unsigned long long uintmax_t;
#endif

/* intptr_t / uintptr_t (ILP32) */
#ifndef _INTPTR_T
#define _INTPTR_T
typedef int                     intptr_t;
#endif
#ifndef _UINTPTR_T
#define _UINTPTR_T
typedef unsigned int            uintptr_t;
#endif

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
 * ================================================================ */
#ifndef _INT_LEAST8_T
#define _INT_LEAST8_T
typedef signed char      int_least8_t;
#endif
#ifndef _INT_LEAST16_T
#define _INT_LEAST16_T
typedef short            int_least16_t;
#endif
#ifndef _INT_LEAST32_T
#define _INT_LEAST32_T
typedef int              int_least32_t;
#endif
#ifndef _INT_LEAST64_T
#define _INT_LEAST64_T
__extension__ typedef long long int_least64_t;
#endif
#ifndef _UINT_LEAST8_T
#define _UINT_LEAST8_T
typedef unsigned char    uint_least8_t;
#endif
#ifndef _UINT_LEAST16_T
#define _UINT_LEAST16_T
typedef unsigned short   uint_least16_t;
#endif
#ifndef _UINT_LEAST32_T
#define _UINT_LEAST32_T
typedef unsigned int     uint_least32_t;
#endif
#ifndef _UINT_LEAST64_T
#define _UINT_LEAST64_T
__extension__ typedef unsigned long long uint_least64_t;
#endif

/* ================================================================
 * Fastest minimum-width integer types
 * (SPARC favors 32-bit alignment for narrow types)
 * ================================================================ */
#ifndef _INT_FAST8_T
#define _INT_FAST8_T
typedef int              int_fast8_t;
#endif
#ifndef _INT_FAST16_T
#define _INT_FAST16_T
typedef int              int_fast16_t;
#endif
#ifndef _INT_FAST32_T
#define _INT_FAST32_T
typedef int              int_fast32_t;
#endif
#ifndef _INT_FAST64_T
#define _INT_FAST64_T
__extension__ typedef long long int_fast64_t;
#endif
#ifndef _UINT_FAST8_T
#define _UINT_FAST8_T
typedef unsigned int     uint_fast8_t;
#endif
#ifndef _UINT_FAST16_T
#define _UINT_FAST16_T
typedef unsigned int     uint_fast16_t;
#endif
#ifndef _UINT_FAST32_T
#define _UINT_FAST32_T
typedef unsigned int     uint_fast32_t;
#endif
#ifndef _UINT_FAST64_T
#define _UINT_FAST64_T
__extension__ typedef unsigned long long uint_fast64_t;
#endif

#endif /* _SOLCOMPAT_OVERRIDE_STDINT_H */
