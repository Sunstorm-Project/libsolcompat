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

#ifdef __sun
#include <solcompat/snprintf.h>
#include <solcompat/stdio_ext.h>

/*
 * C99 variadic scanf functions.
 * Solaris 7 libc may have these internally but doesn't declare them.
 * Provide declarations so C99-conformant code compiles.
 */
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif
extern int vfscanf(FILE *, const char *, va_list);
extern int vscanf(const char *, va_list);
extern int vsscanf(const char *, const char *, va_list);
#ifdef __cplusplus
}
#endif
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_STDIO_H */
