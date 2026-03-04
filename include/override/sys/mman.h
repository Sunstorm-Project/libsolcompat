/*
 * override/sys/mman.h — MAP_ANONYMOUS support for Solaris 7
 *
 * Solaris 7's mman.h lacks MAP_ANONYMOUS.  This wrapper pulls in the
 * real header via #include_next, then adds the flag and a transparent
 * mmap wrapper that uses /dev/zero for anonymous mappings.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_SYS_MMAN_H
#define _SOLCOMPAT_OVERRIDE_SYS_MMAN_H

/* Pull in the real Solaris 7 /usr/include/sys/mman.h */
#include_next <sys/mman.h>

/* Add MAP_ANONYMOUS compatibility */
#include <solcompat/memory.h>

#endif /* _SOLCOMPAT_OVERRIDE_SYS_MMAN_H */
