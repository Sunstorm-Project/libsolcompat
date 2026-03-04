/*
 * override/locale.h — POSIX 2008 locale extensions for Solaris 7
 *
 * Solaris 7's locale.h lacks newlocale/uselocale/freelocale/duplocale.
 * This wrapper pulls in the real header, then adds the missing APIs.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_LOCALE_H
#define _SOLCOMPAT_OVERRIDE_LOCALE_H

/* Pull in the real Solaris 7 /usr/include/locale.h */
#include_next <locale.h>

/* Add POSIX 2008 per-thread locale stubs */
#include <solcompat/stubs.h>

#endif /* _SOLCOMPAT_OVERRIDE_LOCALE_H */
