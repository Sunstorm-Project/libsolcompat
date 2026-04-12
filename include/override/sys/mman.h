/*
 * override/sys/mman.h — MAP_ANONYMOUS + POSIX munmap for Solaris 7
 *
 * Solaris 7's mman.h lacks MAP_ANONYMOUS and declares munmap(caddr_t, ...)
 * instead of the POSIX munmap(void *, ...).  This wrapper adds both.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_SYS_MMAN_H
#define _SOLCOMPAT_OVERRIDE_SYS_MMAN_H

/* Pull in the real Solaris 7 /usr/include/sys/mman.h */
#include_next <sys/mman.h>

#ifdef __sun
#include <solcompat/memory.h>

/*
 * Solaris 7 declares munmap(caddr_t, size_t) where caddr_t is char*.
 * POSIX and modern code expects munmap(void*, size_t).
 * In C++ mode, provide an inline overload that accepts void*.
 */
#ifdef __cplusplus
static inline int munmap(void *addr, size_t len) {
    return munmap(static_cast<caddr_t>(addr), len);
}
#endif
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_SYS_MMAN_H */
