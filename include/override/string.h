/*
 * override/string.h — POSIX 2008 string extensions for Solaris 7
 *
 * Solaris 7's string.h lacks strndup, strnlen, strsignal, etc.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_STRING_H
#define _SOLCOMPAT_OVERRIDE_STRING_H

/* Pull in the real Solaris 7 /usr/include/string.h */
#include_next <string.h>

#ifdef __sun
#include <solcompat/string_ext.h>
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_STRING_H */
