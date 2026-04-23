/*
 * complex_math.c — C99 complex math stubs for Solaris 7
 *
 * GCC optimizes paired sin(x)+cos(x) calls into cexpf(ix) using
 * Euler's formula. Solaris 7 has no <complex.h> or complex math
 * library, so we provide cexpf here using GCC's __complex__ extension
 * (available even without <complex.h>).
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */
#include <math.h>

__complex__ float cexpf(__complex__ float z) {
    float real_part = __real__ z;
    float imag_part = __imag__ z;
    float exp_real = expf(real_part);
    __complex__ float result;
    __real__ result = exp_real * cosf(imag_part);
    __imag__ result = exp_real * sinf(imag_part);
    return result;
}
/* cexp (double complex) already lives in src/math.c */
