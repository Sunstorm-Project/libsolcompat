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
 * Guard strategy: each type is guarded by BOTH the traditional Solaris
 * guard (e.g. _INT8_T) AND the GCC-standard guard (__int8_t_defined),
 * whichever was set first.  This prevents conflicts when GCC 11+
 * include-fixed headers define these types before this file is reached.
 *
 * Part of libsolcompat -- https://github.com/Sunstorm-Project/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_STDINT_H
#define _SOLCOMPAT_OVERRIDE_STDINT_H

/*
 * Prevent sys/int_types.h from being included after us.  That header
 * redefines intmax_t as int32_t under __STRICT_ANSI__ (i.e. -std=c99),
 * conflicting with our correct C99 long long definition.  Since we
 * provide a complete superset of its types, suppress it entirely.
 */
#ifndef _SYS_INT_TYPES_H
#define _SYS_INT_TYPES_H
#endif

/* ================================================================
 * Exact-width signed types
 * Skipped when GCC include-fixed sys/int_types.h already provided them.
 * ================================================================ */
#if !defined(__int8_t_defined) && !defined(_INT8_T)
#  define __int8_t_defined
#  define _INT8_T
  /*
   * Use plain 'char' (not 'signed char') to match GCC include-fixed
   * sys/int_types.h which typedefs int8_t as 'char'.  Both are 8-bit
   * signed on SPARC; using the same underlying type avoids a
   * 'conflicting types' error regardless of include order.
   */
#  ifdef __INT8_TYPE__
typedef __INT8_TYPE__  int8_t;
#  else
typedef char           int8_t;
#  endif
#endif

#if !defined(_INT16_T)
#  define _INT16_T
#  ifdef __INT16_TYPE__
typedef __INT16_TYPE__ int16_t;
#  else
typedef short          int16_t;
#  endif
#endif

#if !defined(_INT32_T)
#  define _INT32_T
#  ifdef __INT32_TYPE__
typedef __INT32_TYPE__ int32_t;
#  else
typedef int            int32_t;
#  endif
#endif

#if !defined(_INT64_T)
#  define _INT64_T
#  ifdef __INT64_TYPE__
typedef __INT64_TYPE__ int64_t;
#  else
__extension__ typedef long long int64_t;
#  endif
#endif

/* ----------------------------------------------------------------
 * Exact-width unsigned types
 * ---------------------------------------------------------------- */
#if !defined(_UINT8_T)
#  define _UINT8_T
#  ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__  uint8_t;
#  else
typedef unsigned char   uint8_t;
#  endif
#endif

#if !defined(_UINT16_T)
#  define _UINT16_T
#  ifdef __UINT16_TYPE__
typedef __UINT16_TYPE__ uint16_t;
#  else
typedef unsigned short  uint16_t;
#  endif
#endif

#if !defined(_UINT32_T)
#  define _UINT32_T
#  ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ uint32_t;
#  else
typedef unsigned int    uint32_t;
#  endif
#endif

#if !defined(_UINT64_T)
#  define _UINT64_T
#  ifdef __UINT64_TYPE__
typedef __UINT64_TYPE__ uint64_t;
#  else
__extension__ typedef unsigned long long uint64_t;
#  endif
#endif

/* ----------------------------------------------------------------
 * intmax_t / uintmax_t
 * ---------------------------------------------------------------- */
#if !defined(__intmax_t_defined) && !defined(_INTMAX_T)
#  define __intmax_t_defined
#  define _INTMAX_T
#  ifdef __INTMAX_TYPE__
typedef __INTMAX_TYPE__ intmax_t;
#  else
__extension__ typedef long long intmax_t;
#  endif
#endif

#if !defined(_UINTMAX_T)
#  define _UINTMAX_T
#  ifdef __UINTMAX_TYPE__
typedef __UINTMAX_TYPE__ uintmax_t;
#  else
__extension__ typedef unsigned long long uintmax_t;
#  endif
#endif

/* ----------------------------------------------------------------
 * intptr_t / uintptr_t  (ILP32 SPARC)
 * ---------------------------------------------------------------- */
#if !defined(__intptr_t_defined) && !defined(_INTPTR_T)
#  define __intptr_t_defined
#  define _INTPTR_T
#  ifdef __INTPTR_TYPE__
typedef __INTPTR_TYPE__ intptr_t;
#  else
typedef int             intptr_t;
#  endif
#endif

#if !defined(_UINTPTR_T)
#  define _UINTPTR_T
#  ifdef __UINTPTR_TYPE__
typedef __UINTPTR_TYPE__ uintptr_t;
#  else
typedef unsigned int     uintptr_t;
#  endif
#endif

/* ================================================================
 * Exact-width integer limits
 * ================================================================ */
#ifndef INT8_MIN
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
#endif

/* ================================================================
 * Minimum-width integer types
 * ================================================================ */
#if !defined(__int_least8_t_defined) && !defined(_INT_LEAST8_T)
#  define __int_least8_t_defined
#  define _INT_LEAST8_T
  /* Use plain 'char' to match GCC include-fixed sys/int_types.h. */
#  ifdef __INT_LEAST8_TYPE__
typedef __INT_LEAST8_TYPE__  int_least8_t;
#  else
typedef char                 int_least8_t;
#  endif
#endif

#if !defined(_INT_LEAST16_T)
#  define _INT_LEAST16_T
#  ifdef __INT_LEAST16_TYPE__
typedef __INT_LEAST16_TYPE__ int_least16_t;
#  else
typedef short                int_least16_t;
#  endif
#endif

#if !defined(_INT_LEAST32_T)
#  define _INT_LEAST32_T
#  ifdef __INT_LEAST32_TYPE__
typedef __INT_LEAST32_TYPE__ int_least32_t;
#  else
typedef int                  int_least32_t;
#  endif
#endif

#if !defined(_INT_LEAST64_T)
#  define _INT_LEAST64_T
#  ifdef __INT_LEAST64_TYPE__
typedef __INT_LEAST64_TYPE__ int_least64_t;
#  else
__extension__ typedef long long int_least64_t;
#  endif
#endif

#if !defined(_UINT_LEAST8_T)
#  define _UINT_LEAST8_T
#  ifdef __UINT_LEAST8_TYPE__
typedef __UINT_LEAST8_TYPE__  uint_least8_t;
#  else
typedef unsigned char         uint_least8_t;
#  endif
#endif

#if !defined(_UINT_LEAST16_T)
#  define _UINT_LEAST16_T
#  ifdef __UINT_LEAST16_TYPE__
typedef __UINT_LEAST16_TYPE__ uint_least16_t;
#  else
typedef unsigned short        uint_least16_t;
#  endif
#endif

#if !defined(_UINT_LEAST32_T)
#  define _UINT_LEAST32_T
#  ifdef __UINT_LEAST32_TYPE__
typedef __UINT_LEAST32_TYPE__ uint_least32_t;
#  else
typedef unsigned int          uint_least32_t;
#  endif
#endif

#if !defined(_UINT_LEAST64_T)
#  define _UINT_LEAST64_T
#  ifdef __UINT_LEAST64_TYPE__
typedef __UINT_LEAST64_TYPE__ uint_least64_t;
#  else
__extension__ typedef unsigned long long uint_least64_t;
#  endif
#endif

/* ================================================================
 * Minimum-width integer limits
 * ================================================================ */
#ifndef INT_LEAST8_MIN
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
#endif

/* ================================================================
 * Fastest minimum-width integer types
 * (SPARC favors 32-bit alignment for narrow types)
 * ================================================================ */
#if !defined(__int_fast8_t_defined) && !defined(_INT_FAST8_T)
#  define __int_fast8_t_defined
#  define _INT_FAST8_T
#  ifdef __INT_FAST8_TYPE__
typedef __INT_FAST8_TYPE__  int_fast8_t;
#  else
typedef int                 int_fast8_t;
#  endif
#endif

#ifndef _INT_FAST16_T
#  define _INT_FAST16_T
#  ifdef __INT_FAST16_TYPE__
typedef __INT_FAST16_TYPE__ int_fast16_t;
#  else
typedef int                 int_fast16_t;
#  endif
#endif

#ifndef _INT_FAST32_T
#  define _INT_FAST32_T
#  ifdef __INT_FAST32_TYPE__
typedef __INT_FAST32_TYPE__ int_fast32_t;
#  else
typedef int                 int_fast32_t;
#  endif
#endif

#ifndef _INT_FAST64_T
#  define _INT_FAST64_T
#  ifdef __INT_FAST64_TYPE__
typedef __INT_FAST64_TYPE__ int_fast64_t;
#  else
__extension__ typedef long long int_fast64_t;
#  endif
#endif

#ifndef _UINT_FAST8_T
#  define _UINT_FAST8_T
#  ifdef __UINT_FAST8_TYPE__
typedef __UINT_FAST8_TYPE__  uint_fast8_t;
#  else
typedef unsigned int         uint_fast8_t;
#  endif
#endif

#ifndef _UINT_FAST16_T
#  define _UINT_FAST16_T
#  ifdef __UINT_FAST16_TYPE__
typedef __UINT_FAST16_TYPE__ uint_fast16_t;
#  else
typedef unsigned int         uint_fast16_t;
#  endif
#endif

#ifndef _UINT_FAST32_T
#  define _UINT_FAST32_T
#  ifdef __UINT_FAST32_TYPE__
typedef __UINT_FAST32_TYPE__ uint_fast32_t;
#  else
typedef unsigned int         uint_fast32_t;
#  endif
#endif

#ifndef _UINT_FAST64_T
#  define _UINT_FAST64_T
#  ifdef __UINT_FAST64_TYPE__
typedef __UINT_FAST64_TYPE__ uint_fast64_t;
#  else
__extension__ typedef unsigned long long uint_fast64_t;
#  endif
#endif

/* ================================================================
 * Fastest minimum-width integer limits
 * ================================================================ */
#ifndef INT_FAST8_MIN
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
#endif

/* ================================================================
 * Pointer-width integer limits (ILP32 — SPARC 32-bit)
 * ================================================================ */
#ifndef INTPTR_MIN
#define INTPTR_MIN   INT32_MIN
#define INTPTR_MAX   INT32_MAX
#define UINTPTR_MAX  UINT32_MAX
#endif

/* ================================================================
 * Maximum-width integer limits
 * ================================================================ */
#ifndef INTMAX_MIN
#define INTMAX_MIN   INT64_MIN
#define INTMAX_MAX   INT64_MAX
#define UINTMAX_MAX  UINT64_MAX
#endif

/* ================================================================
 * Other limits
 * ================================================================ */
#ifndef PTRDIFF_MIN
#define PTRDIFF_MIN  INT32_MIN
#define PTRDIFF_MAX  INT32_MAX
#define SIZE_MAX     UINT32_MAX
#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX
#define WCHAR_MIN    0
#define WCHAR_MAX    2147483647
#define WINT_MIN     INT32_MIN
#define WINT_MAX     INT32_MAX
#endif

/* ================================================================
 * Integer constant macros
 * ================================================================ */
#ifndef INT8_C
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
#endif

#endif /* _SOLCOMPAT_OVERRIDE_STDINT_H */
