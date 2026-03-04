/*
 * solcompat/compat_types.h — Type compatibility for Solaris 7
 *
 * Solaris 7 uses older BSD-style types in some system call prototypes:
 *   - munmap(caddr_t addr, ...) instead of munmap(void *addr, ...)
 *   - mmap returns caddr_t instead of void*
 *
 * This header provides cast macros so portable C++ code compiles
 * without implicit void* → char* conversion errors.
 */
#ifndef SOLCOMPAT_COMPAT_TYPES_H
#define SOLCOMPAT_COMPAT_TYPES_H

#include <sys/types.h>   /* caddr_t */

/*
 * SOLCOMPAT_CADDR(ptr) — Cast any pointer to caddr_t (char*).
 * Use this when calling Solaris 7 APIs that take caddr_t parameters.
 *
 * Example: munmap(SOLCOMPAT_CADDR(buffer), length);
 */
#define SOLCOMPAT_CADDR(ptr) ((caddr_t)(ptr))

/*
 * SOLCOMPAT_VOIDPTR(ptr) — Cast caddr_t or char* to void*.
 * Use this when assigning Solaris 7 return values to void* variables.
 *
 * Example: void *p = SOLCOMPAT_VOIDPTR(mmap(0, len, ...));
 */
#define SOLCOMPAT_VOIDPTR(ptr) ((void *)(ptr))

#endif /* SOLCOMPAT_COMPAT_TYPES_H */
