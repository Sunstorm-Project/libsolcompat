/*
 * complex.h — C99 complex arithmetic header for Solaris 7
 *
 * Solaris 7 predates C99 and ships no <complex.h>. mpc 1.4.0 and any
 * other C99 library that uses `#include <complex.h>` unconditionally
 * fails the compile at "complex.h: No such file or directory".
 *
 * GCC's `_Complex` keyword and its __real__, __imag__, __builtin_conj,
 * __builtin_creal, __builtin_cimag intrinsics supply everything C99
 * needs for basic complex arithmetic at compile time — no runtime
 * library is required for the projection operators (creal/cimag/conj)
 * or for the type qualifier (complex). This header labels the
 * intrinsics with their C99 names so code that only uses these parts
 * of the standard compiles and links cleanly.
 *
 * The full C99 complex math library (cexp, cpow, clog, csqrt, ...) is
 * NOT declared here.  libsolcompat src/complex_math.c currently only
 * provides a cexpf stub for GCC's sin/cos fusion optimization, not a
 * real complex math library.  Consumers that need the full family
 * (none known at the time of writing) should either add the decls
 * here and the implementations to libsolcompat, or link against a
 * third-party C99 complex math library.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _COMPLEX_H
#define _COMPLEX_H

/* C99 7.3.1 — complex and imaginary type qualifier macros.
 * GCC supports `_Complex` natively in C99 mode (and as an
 * extension in C89). `_Imaginary` is optional in C99; no mainstream
 * compiler implements it, but we define the macro for source
 * compatibility with code that references it. */
#define complex   _Complex
#define imaginary _Imaginary

/* C99 7.3.1 — imaginary unit constant. GCC's `1.0iF` suffix is a
 * GNU extension that produces a `float _Complex` value equal to i. */
#ifdef __GNUC__
# define _Complex_I (__extension__ 1.0iF)
#else
# define _Complex_I ((const float _Complex){0.0f, 1.0f})
#endif

#define I _Complex_I

/* C99 7.3.9.4 — creal / cimag: real and imaginary part projections.
 * GCC's __real__ and __imag__ operators return the respective lvalue
 * component of a `_Complex` expression. They are pure compile-time
 * operators — no runtime library call. */
#define creal(z)  __real__(z)
#define crealf(z) __real__(z)
#define creall(z) __real__(z)

#define cimag(z)  __imag__(z)
#define cimagf(z) __imag__(z)
#define cimagl(z) __imag__(z)

/* C99 7.3.9.3 — conj: complex conjugate. GCC provides
 * __builtin_conj, __builtin_conjf, __builtin_conjl that lower to
 * a sign flip on the imaginary part at compile time. */
#define conj(z)  __builtin_conj(z)
#define conjf(z) __builtin_conjf(z)
#define conjl(z) __builtin_conjl(z)

/* C11 7.3.9.3 — CMPLX constructor macros. GCC's __builtin_complex
 * produces a `_Complex` value from two real components without
 * going through the trap-representation arithmetic that `x + I*y`
 * would otherwise require. */
#define CMPLX(x, y)  __builtin_complex((double)(x),      (double)(y))
#define CMPLXF(x, y) __builtin_complex((float)(x),       (float)(y))
#define CMPLXL(x, y) __builtin_complex((long double)(x), (long double)(y))

#endif /* _COMPLEX_H */
