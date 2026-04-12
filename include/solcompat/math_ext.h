/*
 * solcompat/math_ext.h — C99/C11 math functions and macros for Solaris 7
 *
 * Solaris 7's libm only provides double-precision functions.
 * This header declares all missing C99 float and long double variants
 * (fabsf, fabsl, sqrtf, sqrtl, etc.), plus missing double functions
 * (tgamma, round, trunc, log2, exp2, ...), fp classification macros,
 * and __fpclassify functions needed by GCC's __builtin_fpclassify.
 *
 * Implementations are in src/math.c
 */
#ifndef SOLCOMPAT_MATH_EXT_H
#define SOLCOMPAT_MATH_EXT_H

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * C99 constant macros
 * ================================================================ */

#ifndef INFINITY
#define INFINITY (__builtin_inff())
#endif

#ifndef NAN
#define NAN (__builtin_nanf(""))
#endif

#ifndef HUGE_VALF
#define HUGE_VALF INFINITY
#endif

#ifndef HUGE_VALL
#define HUGE_VALL ((long double)HUGE_VAL)
#endif

/* ================================================================
 * C99 floating-point classification macros
 * These are the integer constants used by __builtin_fpclassify
 * ================================================================ */

#ifndef FP_NAN
#define FP_NAN       0
#endif
#ifndef FP_INFINITE
#define FP_INFINITE  1
#endif
#ifndef FP_ZERO
#define FP_ZERO      2
#endif
#ifndef FP_SUBNORMAL
#define FP_SUBNORMAL 3
#endif
#ifndef FP_NORMAL
#define FP_NORMAL    4
#endif

/* fpclassify — Solaris 7 has fpclass() in ieeefp.h but not C99 fpclassify */
int solcompat_fpclassify_d(double x);
int solcompat_fpclassify_f(float x);

#ifndef __cplusplus
#ifndef fpclassify
#define fpclassify(x) \
    (sizeof(x) == sizeof(float) ? solcompat_fpclassify_f(x) : solcompat_fpclassify_d(x))
#endif
#endif /* !__cplusplus */

/* GCC's __builtin_fpclassify needs these on some targets */
int __fpclassifyf(float x);
int __fpclassifyd(double x);

/* C99 isnan/isinf/isfinite/isnormal macros (type-generic).
 * In C++ mode, GCC provides these as builtins via <cmath> so we only
 * define them for C.  Defining them as macros in C++ breaks std::isinf() etc. */
#ifndef __cplusplus
#ifndef isfinite
#define isfinite(x) (fpclassify(x) != FP_NAN && fpclassify(x) != FP_INFINITE)
#endif
#ifndef isinf
#define isinf(x)    (fpclassify(x) == FP_INFINITE)
#endif
#ifndef isnormal
#define isnormal(x) (fpclassify(x) == FP_NORMAL)
#endif
#endif /* !__cplusplus */

/* ================================================================
 * Missing double-precision C99 math functions
 * (Solaris 7 libm lacks these entirely)
 * ================================================================ */

double round(double x);
double trunc(double x);
double log2(double x);
double exp2(double x);
double fdim(double x, double y);
double fmin(double x, double y);
double fmax(double x, double y);
double tgamma(double x);
double nearbyint(double x);
long int lrint(double x);
long int lround(double x);
long long int llrint(double x);
long long int llround(double x);
double nan(const char *tagp);
double remquo(double x, double y, int *quo);
double scalbln(double x, long int n);
/* fma/fmaf/fmal declared in src/math.c only — GCC 15 treats any
 * declaration of these builtins as an implicit inline definition,
 * causing a redefinition error when the explicit implementation
 * appears later.  The C++ declarations live in override/cmath. */

/* ================================================================
 * C99 float-precision math functions (ALL missing from Solaris 7)
 * ================================================================ */

/* Unary float */
float fabsf(float x);
float sqrtf(float x);
float sinf(float x);
float cosf(float x);
float tanf(float x);
float asinf(float x);
float acosf(float x);
float atanf(float x);
float sinhf(float x);
float coshf(float x);
float tanhf(float x);
float asinhf(float x);
float acoshf(float x);
float atanhf(float x);
float expf(float x);
float expm1f(float x);
float exp2f(float x);
float logf(float x);
float log10f(float x);
float log1pf(float x);
float log2f(float x);
float logbf(float x);
float cbrtf(float x);
float ceilf(float x);
float floorf(float x);
float truncf(float x);
float roundf(float x);
float rintf(float x);
float nearbyintf(float x);
float erff(float x);
float erfcf(float x);
float lgammaf(float x);
float tgammaf(float x);
int   ilogbf(float x);
float nanf(const char *tagp);

/* Binary float */
float powf(float x, float y);
float fmodf(float x, float y);
float atan2f(float x, float y);
float copysignf(float x, float y);
float nextafterf(float x, float y);
float remainderf(float x, float y);
float hypotf(float x, float y);
float fdimf(float x, float y);
float fminf(float x, float y);
float fmaxf(float x, float y);
float scalbnf(float x, int n);
float scalblnf(float x, long int n);

/* Special float */
float ldexpf(float x, int e);
float frexpf(float x, int *e);
float modff(float x, float *iptr);

/* Ternary float */
float fmaf(float x, float y, float z);

/* Rounding / integer conversion */
long int lrintf(float x);
long int lroundf(float x);
long long int llrintf(float x);
long long int llroundf(float x);

/* ================================================================
 * C99 long double math functions (ALL missing from Solaris 7)
 * Solaris 7 SPARC: long double == double (both 64-bit IEEE 754),
 * so these are trivial wrappers.
 * ================================================================ */

/* Unary long double */
long double fabsl(long double x);
long double sqrtl(long double x);
long double sinl(long double x);
long double cosl(long double x);
long double tanl(long double x);
long double asinl(long double x);
long double acosl(long double x);
long double atanl(long double x);
long double sinhl(long double x);
long double coshl(long double x);
long double tanhl(long double x);
long double asinhl(long double x);
long double acoshl(long double x);
long double atanhl(long double x);
long double expl(long double x);
long double expm1l(long double x);
long double exp2l(long double x);
long double logl(long double x);
long double log10l(long double x);
long double log1pl(long double x);
long double log2l(long double x);
long double logbl(long double x);
long double cbrtl(long double x);
long double ceill(long double x);
long double floorl(long double x);
long double truncl(long double x);
long double roundl(long double x);
long double rintl(long double x);
long double nearbyintl(long double x);
long double erfl(long double x);
long double erfcl(long double x);
long double lgammal(long double x);
long double tgammal(long double x);
int         ilogbl(long double x);
long double nanl(const char *tagp);

/* Binary long double */
long double powl(long double x, long double y);
long double fmodl(long double x, long double y);
long double atan2l(long double x, long double y);
long double copysignl(long double x, long double y);
long double nextafterl(long double x, long double y);
long double remainderl(long double x, long double y);
long double hypotl(long double x, long double y);
long double fdiml(long double x, long double y);
long double fminl(long double x, long double y);
long double fmaxl(long double x, long double y);
long double scalbnl(long double x, int n);
long double scalblnl(long double x, long int n);

/* Special long double */
long double ldexpl(long double x, int e);
long double frexpl(long double x, int *e);
long double modfl(long double x, long double *iptr);

/* Ternary long double */
long double fmal(long double x, long double y, long double z);

/* Rounding / integer conversion */
long int lrintl(long double x);
long int lroundl(long double x);
long long int llrintl(long double x);
long long int llroundl(long double x);

/* ================================================================
 * C99 complex math
 * Solaris 7 libm has no complex math support at all.
 * ================================================================ */

/*
 * cexp — complex exponential.  Used by libsvgtiny for SVG arc paths.
 * cexp(a+bi) = e^a * (cos(b) + i*sin(b))
 */
double _Complex cexp(double _Complex z);

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_MATH_EXT_H */
