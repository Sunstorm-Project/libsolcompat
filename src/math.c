/*
 * math.c — Comprehensive C99 math functions for Solaris 7
 *
 * Solaris 7's libm only has double-precision versions of most math
 * functions. This file provides:
 *   - All missing double-precision C99 functions (round, trunc, log2,
 *     exp2, tgamma, nearbyint, lrint, lround, nan, remquo, scalbln, etc.)
 *   - ALL float-precision (f-suffix) wrappers
 *   - ALL long-double-precision (l-suffix) wrappers
 *     (Solaris 7 SPARC: long double == double, both 64-bit IEEE 754)
 *   - fpclassify / __fpclassifyf / __fpclassifyd helpers
 */

#include <math.h>
#include <float.h>
#include <string.h>
#include <errno.h>
#include <ieeefp.h>   /* Solaris: finite(), isnand(), fpclass(), signgam */

/* ================================================================
 * fpclassify helpers
 * ================================================================ */

int
solcompat_fpclassify_d(double x)
{
    fpclass_t c = fpclass(x);
    switch (c) {
    case FP_SNAN:
    case FP_QNAN:
        return 0; /* FP_NAN */
    case FP_NINF:
    case FP_PINF:
        return 1; /* FP_INFINITE */
    case FP_NZERO:
    case FP_PZERO:
        return 2; /* FP_ZERO */
    case FP_NDENORM:
    case FP_PDENORM:
        return 3; /* FP_SUBNORMAL */
    case FP_NNORM:
    case FP_PNORM:
    default:
        return 4; /* FP_NORMAL */
    }
}

int
solcompat_fpclassify_f(float x)
{
    return solcompat_fpclassify_d((double)x);
}

int __fpclassifyf(float x)  { return solcompat_fpclassify_f(x); }
int __fpclassifyd(double x)  { return solcompat_fpclassify_d(x); }

/* ================================================================
 * Missing double-precision functions
 * ================================================================ */

double
round(double x)
{
    if (x >= 0.0)
        return floor(x + 0.5);
    else
        return ceil(x - 0.5);
}

double
trunc(double x)
{
    return (x >= 0.0) ? floor(x) : ceil(x);
}

double
log2(double x)
{
    return log(x) / 0.6931471805599453094172321;  /* 1/ln(2) */
}

double
exp2(double x)
{
    return pow(2.0, x);
}

double
fdim(double x, double y)
{
    return (x > y) ? (x - y) : 0.0;
}

double
fmin(double x, double y)
{
    if (isnand(x)) return y;
    if (isnand(y)) return x;
    return (x < y) ? x : y;
}

double
fmax(double x, double y)
{
    if (isnand(x)) return y;
    if (isnand(y)) return x;
    return (x > y) ? x : y;
}

double
tgamma(double x)
{
    /*
     * tgamma via lgamma + signgam.
     * Not perfectly accurate for edge cases but sufficient for cross-build.
     */
    extern int signgam;
    double lg = lgamma(x);
    if (!finite(lg))
        return lg;
    return (double)signgam * exp(lg);
}

double
nearbyint(double x)
{
    return rint(x);  /* Solaris 7 has rint() */
}

long int
lrint(double x)
{
    return (long int)rint(x);
}

long int
lround(double x)
{
    return (long int)round(x);
}

long long int
llrint(double x)
{
    return (long long int)rint(x);
}

long long int
llround(double x)
{
    return (long long int)round(x);
}

double
nan(const char *tagp)
{
    (void)tagp;
    return 0.0 / 0.0;
}

double
remquo(double x, double y, int *quo)
{
    double r = remainder(x, y);
    /* Compute low-order bits of quotient */
    if (quo) {
        double q = (x - r) / y;
        *quo = (int)q & 0x7;
        if ((x < 0.0) != (y < 0.0))
            *quo = -(*quo);
    }
    return r;
}

double
scalbln(double x, long int n)
{
    return scalbn(x, (int)n);
}

/* Use volatile to prevent GCC from recognizing the pattern as __builtin_fma
   and optimizing it into a recursive call to fma() itself. */
double
fma(double x, double y, double z)
{
    volatile double product = x * y;
    return product + z;
}

/* ================================================================
 * Float wrappers — cast through double
 * ================================================================ */

float fabsf(float x)  { return (float)fabs((double)x); }
float sqrtf(float x)  { return (float)sqrt((double)x); }
float sinf(float x)   { return (float)sin((double)x); }
float cosf(float x)   { return (float)cos((double)x); }
float tanf(float x)   { return (float)tan((double)x); }
float asinf(float x)  { return (float)asin((double)x); }
float acosf(float x)  { return (float)acos((double)x); }
float atanf(float x)  { return (float)atan((double)x); }
float sinhf(float x)  { return (float)sinh((double)x); }
float coshf(float x)  { return (float)cosh((double)x); }
float tanhf(float x)  { return (float)tanh((double)x); }
float asinhf(float x) { return (float)asinh((double)x); }
float acoshf(float x) { return (float)acosh((double)x); }
float atanhf(float x) { return (float)atanh((double)x); }
float expf(float x)   { return (float)exp((double)x); }
float expm1f(float x) { return (float)expm1((double)x); }
float exp2f(float x)  { return (float)exp2((double)x); }
float logf(float x)   { return (float)log((double)x); }
float log10f(float x) { return (float)log10((double)x); }
float log1pf(float x) { return (float)log1p((double)x); }
float log2f(float x)  { return (float)log2((double)x); }
float logbf(float x)  { return (float)logb((double)x); }
float cbrtf(float x)  { return (float)cbrt((double)x); }
float ceilf(float x)  { return (float)ceil((double)x); }
float floorf(float x) { return (float)floor((double)x); }
float truncf(float x) { return (float)trunc((double)x); }
float roundf(float x) { return (float)round((double)x); }
float rintf(float x)   { return (float)rint((double)x); }
float nearbyintf(float x) { return (float)nearbyint((double)x); }
float erff(float x)    { return (float)erf((double)x); }
float erfcf(float x)   { return (float)erfc((double)x); }
float lgammaf(float x) { return (float)lgamma((double)x); }
float tgammaf(float x) { return (float)tgamma((double)x); }
int   ilogbf(float x)  { return ilogb((double)x); }
float nanf(const char *tagp) { (void)tagp; return (float)(0.0f / 0.0f); }

/* Binary float */
float powf(float x, float y)        { return (float)pow((double)x, (double)y); }
float fmodf(float x, float y)       { return (float)fmod((double)x, (double)y); }
float atan2f(float x, float y)      { return (float)atan2((double)x, (double)y); }
float copysignf(float x, float y)   { return (float)copysign((double)x, (double)y); }
float nextafterf(float x, float y)  { return (float)nextafter((double)x, (double)y); }
float remainderf(float x, float y)  { return (float)remainder((double)x, (double)y); }
float hypotf(float x, float y)      { return (float)hypot((double)x, (double)y); }
float fdimf(float x, float y)       { return (float)fdim((double)x, (double)y); }
float fminf(float x, float y)       { return (float)fmin((double)x, (double)y); }
float fmaxf(float x, float y)       { return (float)fmax((double)x, (double)y); }
float scalbnf(float x, int n)       { return (float)scalbn((double)x, n); }
float scalblnf(float x, long int n) { return (float)scalbln((double)x, n); }
float fmaf(float x, float y, float z) { volatile double r = fma((double)x, (double)y, (double)z); return (float)r; }

/* Special float */
float ldexpf(float x, int e)     { return (float)ldexp((double)x, e); }
float frexpf(float x, int *e)    { return (float)frexp((double)x, e); }
float modff(float x, float *iptr) {
    double di;
    double r = modf((double)x, &di);
    *iptr = (float)di;
    return (float)r;
}

/* Ternary float */
float fmaf(float x, float y, float z) {
    return (float)((double)x * (double)y + (double)z);
}

/* Rounding / integer conversion */
long int      lrintf(float x)  { return lrint((double)x); }
long int      lroundf(float x) { return lround((double)x); }
long long int llrintf(float x) { return llrint((double)x); }
long long int llroundf(float x){ return llround((double)x); }

/* ================================================================
 * Long double wrappers
 * Solaris 7 SPARC: long double == double (both 64-bit),
 * so these simply cast through double.
 * ================================================================ */

/* Unary long double */
long double fabsl(long double x)  { return (long double)fabs((double)x); }
long double sqrtl(long double x)  { return (long double)sqrt((double)x); }
long double sinl(long double x)   { return (long double)sin((double)x); }
long double cosl(long double x)   { return (long double)cos((double)x); }
long double tanl(long double x)   { return (long double)tan((double)x); }
long double asinl(long double x)  { return (long double)asin((double)x); }
long double acosl(long double x)  { return (long double)acos((double)x); }
long double atanl(long double x)  { return (long double)atan((double)x); }
long double sinhl(long double x)  { return (long double)sinh((double)x); }
long double coshl(long double x)  { return (long double)cosh((double)x); }
long double tanhl(long double x)  { return (long double)tanh((double)x); }
long double asinhl(long double x) { return (long double)asinh((double)x); }
long double acoshl(long double x) { return (long double)acosh((double)x); }
long double atanhl(long double x) { return (long double)atanh((double)x); }
long double expl(long double x)   { return (long double)exp((double)x); }
long double expm1l(long double x) { return (long double)expm1((double)x); }
long double exp2l(long double x)  { return (long double)exp2((double)x); }
long double logl(long double x)   { return (long double)log((double)x); }
long double log10l(long double x) { return (long double)log10((double)x); }
long double log1pl(long double x) { return (long double)log1p((double)x); }
long double log2l(long double x)  { return (long double)log2((double)x); }
long double logbl(long double x)  { return (long double)logb((double)x); }
long double cbrtl(long double x)  { return (long double)cbrt((double)x); }
long double ceill(long double x)  { return (long double)ceil((double)x); }
long double floorl(long double x) { return (long double)floor((double)x); }
long double truncl(long double x) { return (long double)trunc((double)x); }
long double roundl(long double x) { return (long double)round((double)x); }
long double rintl(long double x)  { return (long double)rint((double)x); }
long double nearbyintl(long double x) { return (long double)nearbyint((double)x); }
long double erfl(long double x)   { return (long double)erf((double)x); }
long double erfcl(long double x)  { return (long double)erfc((double)x); }
long double lgammal(long double x){ return (long double)lgamma((double)x); }
long double tgammal(long double x){ return (long double)tgamma((double)x); }
int         ilogbl(long double x) { return ilogb((double)x); }
long double nanl(const char *tagp){ (void)tagp; return (long double)(0.0 / 0.0); }

/* Binary long double */
long double powl(long double x, long double y)       { return (long double)pow((double)x, (double)y); }
long double fmodl(long double x, long double y)      { return (long double)fmod((double)x, (double)y); }
long double atan2l(long double x, long double y)     { return (long double)atan2((double)x, (double)y); }
long double copysignl(long double x, long double y)  { return (long double)copysign((double)x, (double)y); }
long double nextafterl(long double x, long double y) { return (long double)nextafter((double)x, (double)y); }
long double remainderl(long double x, long double y) { return (long double)remainder((double)x, (double)y); }
long double hypotl(long double x, long double y)     { return (long double)hypot((double)x, (double)y); }
long double fdiml(long double x, long double y)      { return (long double)fdim((double)x, (double)y); }
long double fminl(long double x, long double y)      { return (long double)fmin((double)x, (double)y); }
long double fmaxl(long double x, long double y)      { return (long double)fmax((double)x, (double)y); }
long double scalbnl(long double x, int n)            { return (long double)scalbn((double)x, n); }
long double scalblnl(long double x, long int n)      { return (long double)scalbln((double)x, n); }
long double fmal(long double x, long double y, long double z) { volatile double r = fma((double)x, (double)y, (double)z); return (long double)r; }

/* Special long double */
long double ldexpl(long double x, int e)    { return (long double)ldexp((double)x, e); }
long double frexpl(long double x, int *e)   { return (long double)frexp((double)x, e); }
long double modfl(long double x, long double *iptr) {
    double di;
    double r = modf((double)x, &di);
    *iptr = (long double)di;
    return (long double)r;
}

/* Ternary long double */
long double fmal(long double x, long double y, long double z) {
    return (long double)((double)x * (double)y + (double)z);
}

/* Rounding / integer conversion */
long int      lrintl(long double x)  { return lrint((double)x); }
long int      lroundl(long double x) { return lround((double)x); }
long long int llrintl(long double x) { return llrint((double)x); }
long long int llroundl(long double x){ return llround((double)x); }

/* ================================================================
 * C99 complex math
 * Solaris 7 has no complex math support in libm.
 * ================================================================ */

/*
 * cexp — complex exponential: cexp(a+bi) = e^a * (cos(b) + i*sin(b))
 * Used by libsvgtiny for SVG arc path calculations.
 */
double _Complex
cexp(double _Complex z)
{
    double a = __real__ z;
    double b = __imag__ z;
    double ea = exp(a);
    double _Complex result;
    __real__ result = ea * cos(b);
    __imag__ result = ea * sin(b);
    return result;
}
