/*
 * complex_math.c — C99 complex math stubs for Solaris 7
 *
 * GCC optimizes paired sin(x)+cos(x) calls into cexpf(ix) using
 * Euler's formula. Solaris 7 has no <complex.h> or complex math library.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */
#include <math.h>

typedef struct { float real; float imag; } _Complex_float;

_Complex_float cexpf(_Complex_float z) {
    float expval = expf(z.real);
    _Complex_float result;
    result.real = expval * cosf(z.imag);
    result.imag = expval * sinf(z.imag);
    return result;
}
