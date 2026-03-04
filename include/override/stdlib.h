/*
 * override/stdlib.h — C99/C11 stdlib extensions for Solaris 7
 *
 * Solaris 7's stdlib.h lacks functions added in later POSIX revisions:
 * posix_memalign, mkdtemp, realpath with NULL, etc.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_STDLIB_H
#define _SOLCOMPAT_OVERRIDE_STDLIB_H

/* Pull in the real Solaris 7 /usr/include/stdlib.h */
#include_next <stdlib.h>

/* Add missing stdlib extensions */
#include <solcompat/stdlib_ext.h>

#endif /* _SOLCOMPAT_OVERRIDE_STDLIB_H */
