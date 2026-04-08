/*
 * override/sys/shm.h — POSIX shmdt(void*) for Solaris 7
 *
 * Solaris 7 declares shmdt(char*) instead of shmdt(const void*).
 * Provide a C++ overload that accepts void*.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_SYS_SHM_H
#define _SOLCOMPAT_OVERRIDE_SYS_SHM_H

#include_next <sys/shm.h>

#ifdef __cplusplus
static inline int shmdt(const void *addr) {
    return shmdt(const_cast<char *>(static_cast<const char *>(addr)));
}
#endif

#endif /* _SOLCOMPAT_OVERRIDE_SYS_SHM_H */
