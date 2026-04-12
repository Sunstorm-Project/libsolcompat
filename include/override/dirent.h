/*
 * override/dirent.h — POSIX directory extensions for Solaris 7
 *
 * Solaris 7's <dirent.h> is missing dirfd() and fdopendir().
 * Both are declared in solcompat/filesystem.h and implemented in
 * filesystem.c.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_DIRENT_H
#define _SOLCOMPAT_OVERRIDE_DIRENT_H

/* Pull in the real Solaris 7 /usr/include/dirent.h */
#include_next <dirent.h>

#ifdef __sun
#include <solcompat/filesystem.h>
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_DIRENT_H */
