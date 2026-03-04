/*
 * override/stdio.h — C99 stdio extensions for Solaris 7
 *
 * Solaris 7's stdio.h lacks snprintf/vsnprintf and other C99 additions.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_STDIO_H
#define _SOLCOMPAT_OVERRIDE_STDIO_H

/* Pull in the real Solaris 7 /usr/include/stdio.h */
#include_next <stdio.h>

/* Add missing stdio extensions (snprintf, etc.) */
#include <solcompat/snprintf.h>
#include <solcompat/stdio_ext.h>

#endif /* _SOLCOMPAT_OVERRIDE_STDIO_H */
