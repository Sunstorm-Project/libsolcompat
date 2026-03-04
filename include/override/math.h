/*
 * override/math.h — C99 math compatibility wrapper for Solaris 7
 *
 * This header is installed to an override include directory that GCC
 * searches BEFORE /usr/include (via -isystem in the specs file).
 *
 * It pulls in the real Solaris 7 <math.h> via #include_next, then
 * adds C99 float/long double function declarations and FP classification
 * macros that Solaris 7 lacks.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_MATH_H
#define _SOLCOMPAT_OVERRIDE_MATH_H

/* Pull in the real Solaris 7 /usr/include/math.h */
#include_next <math.h>

/* Add all C99 extensions */
#include <solcompat/math_ext.h>

#endif /* _SOLCOMPAT_OVERRIDE_MATH_H */
