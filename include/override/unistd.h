/*
 * override/unistd.h — POSIX extensions for Solaris 7
 *
 * Solaris 7's <unistd.h> is missing execvpe() and a handful of other
 * functions added in later POSIX revisions.  The missing functions are
 * declared in solcompat/process.h and implemented in process.c.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_UNISTD_H
#define _SOLCOMPAT_OVERRIDE_UNISTD_H

/* Pull in the real Solaris 7 /usr/include/unistd.h */
#include_next <unistd.h>

/* Add execvpe() and related process helpers */
#include <solcompat/process.h>

/* Add *at() functions — openat, fstatat, linkat, renameat, etc. */
#include <solcompat/at_funcs.h>

/* _SC_SYMLOOP_MAX -- not defined on Solaris 7.
 * POSIX says sysconf returns -1 with errno EINVAL for unsupported names,
 * but some code checks the constant at compile time. */
#ifndef _SC_SYMLOOP_MAX
#define _SC_SYMLOOP_MAX 0x200  /* arbitrary unused sysconf name */
#endif

#endif /* _SOLCOMPAT_OVERRIDE_UNISTD_H */
