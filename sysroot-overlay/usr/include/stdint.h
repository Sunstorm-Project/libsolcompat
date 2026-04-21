/*
 * override/stdint.h -- C99 integer types for Solaris 7 SPARC
 *
 * Solaris 7 has no native <stdint.h>.  Its <sys/int_types.h> provides
 * the base integer types, but guards 64-bit types behind
 *   !defined(__STRICT_ANSI__) && !defined(_NO_LONGLONG)
 * which excludes -std=c99/-std=c11 modes.
 *
 * This header temporarily undefines __STRICT_ANSI__ before including
 * <sys/int_types.h> so all types (including 64-bit) are provided,
 * then supplements with fast types and C99 limit/constant macros.
 *
 * Part of libsolcompat -- https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_STDINT_H
#define _SOLCOMPAT_OVERRIDE_STDINT_H

/*
 * Pull in Solaris base types from <sys/int_types.h>.
 *
 * Problem: sys/int_types.h guards int64_t, uint64_t, intmax_t, uintmax_t,
 * int_least64_t, and uint_least64_t behind !__STRICT_ANSI__ (after GCC
 * fixincludes).  When -std=c99 or -std=c11 is used, __STRICT_ANSI__ is
 * defined and those types are omitted — then intmax_t falls back to
 * int32_t, which is wrong for C99 and causes conflicts if we try to
 * redefine it later.
 *
 * Fix: temporarily undefine __STRICT_ANSI__ so sys/int_types.h provides
 * ALL types including 64-bit.  GCC always supports long long, so this
 * is safe.  We restore __STRICT_ANSI__ immediately after.
 */
#ifdef __STRICT_ANSI__
#define _SOLCOMPAT_RESTORE_STRICT_ANSI 1
#undef __STRICT_ANSI__
#endif

#include <sys/int_types.h>

#ifdef _SOLCOMPAT_RESTORE_STRICT_ANSI
#define __STRICT_ANSI__ 1
#undef _SOLCOMPAT_RESTORE_STRICT_ANSI
#endif

/* ================================================================
 * Fastest minimum-width integer types
 * (Not provided by Solaris sys/int_types.h at all)
 * SPARC favors 32-bit alignment for narrow types.
 * ================================================================ */
typedef int              int_fast8_t;
typedef int              int_fast16_t;
typedef int              int_fast32_t;
__extension__ typedef long long int_fast64_t;
typedef unsigned int     uint_fast8_t;
typedef unsigned int     uint_fast16_t;
typedef unsigned int     uint_fast32_t;
__extension__ typedef unsigned long long uint_fast64_t;

/* ================================================================
 * Exact-width integer limits
 * ================================================================ */
/* Per-macro guards — sys/int_limits.h defines the 8/16/32-bit macros
 * unconditionally but gates INT64_MAX/UINT64_MAX on !__STRICT_ANSI__.
 * A single outer `#ifndef INT8_MIN` guard would skip everything when
 * int_limits.h ran first, leaving INT64_MAX undefined in strict modes
 * (surfaced as libxkbcommon build errors). */
#ifndef INT8_MIN
#define INT8_MIN    (-128)
#endif
#ifndef INT8_MAX
#define INT8_MAX    127
#endif
#ifndef UINT8_MAX
#define UINT8_MAX   255U
#endif
#ifndef INT16_MIN
#define INT16_MIN   (-32768)
#endif
#ifndef INT16_MAX
#define INT16_MAX   32767
#endif
#ifndef UINT16_MAX
#define UINT16_MAX  65535U
#endif
#ifndef INT32_MIN
#define INT32_MIN   (-2147483647-1)
#endif
#ifndef INT32_MAX
#define INT32_MAX   2147483647
#endif
#ifndef UINT32_MAX
#define UINT32_MAX  4294967295U
#endif
#ifndef INT64_MIN
#define INT64_MIN   (-9223372036854775807LL-1LL)
#endif
#ifndef INT64_MAX
#define INT64_MAX   9223372036854775807LL
#endif
#ifndef UINT64_MAX
#define UINT64_MAX  18446744073709551615ULL
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
#endif
#ifndef SIZE_MAX
#define SIZE_MAX     UINT32_MAX
#endif
#ifndef SIG_ATOMIC_MIN
#define SIG_ATOMIC_MIN INT32_MIN
#define SIG_ATOMIC_MAX INT32_MAX
#endif
#ifndef WCHAR_MIN
#define WCHAR_MIN    0
#define WCHAR_MAX    2147483647
#endif
#ifndef WINT_MIN
#define WINT_MIN     INT32_MIN
#define WINT_MAX     INT32_MAX
#endif

/* ================================================================
 * Integer constant macros
 * ================================================================ */
/*
 * Per-macro guards (NOT wrapped in one #ifndef INT8_C) because Solaris 7's
 * <sys/int_const.h> defines some of these but gates INT64_C / UINT64_C /
 * INTMAX_C / UINTMAX_C behind `#if __STDC__ - 0 == 0 && !defined(_NO_LONGLONG)`.
 * Under -std=c99 / -std=c11 the compiler sets __STDC__=1 so the Solaris
 * path defines INT8_C / INT32_C / UINT32_C but skips the 64-bit constant
 * macros. A blanket `#ifndef INT8_C` wrapper would then leave INT64_C /
 * UINT64_C / INTMAX_C / UINTMAX_C permanently undefined.
 */
#ifndef INT8_C
#define INT8_C(x)    (x)
#endif
#ifndef UINT8_C
#define UINT8_C(x)   (x ## U)
#endif
#ifndef INT16_C
#define INT16_C(x)   (x)
#endif
#ifndef UINT16_C
#define UINT16_C(x)  (x ## U)
#endif
#ifndef INT32_C
#define INT32_C(x)   (x)
#endif
#ifndef UINT32_C
#define UINT32_C(x)  (x ## U)
#endif
#ifndef INT64_C
#define INT64_C(x)   (x ## LL)
#endif
#ifndef UINT64_C
#define UINT64_C(x)  (x ## ULL)
#endif
#ifndef INTMAX_C
#define INTMAX_C(x)  (x ## LL)
#endif
#ifndef UINTMAX_C
#define UINTMAX_C(x) (x ## ULL)
#endif

#endif /* _SOLCOMPAT_OVERRIDE_STDINT_H */
