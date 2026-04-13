/*
 * sys/random.h — Linux getrandom(2) interface for Solaris 7
 *
 * Solaris 7 has no getrandom(2) syscall and no <sys/random.h>. gnulib's
 * getrandom.c and packages using it (bash, coreutils, Python, git,
 * openssl, openssh, gettext, sudo, wget) include this header
 * unconditionally. libsolcompat's src/random.c provides an
 * implementation that delegates to getentropy() (which reads
 * /dev/urandom).
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SOLCOMPAT_SYS_RANDOM_H
#define _SOLCOMPAT_SYS_RANDOM_H

#include <sys/types.h>   /* ssize_t, size_t */

/* Flags match Linux values; libsolcompat's getrandom ignores them
 * because /dev/urandom is already non-blocking and pseudo-random. */
#ifndef GRND_NONBLOCK
#define GRND_NONBLOCK 0x0001
#endif
#ifndef GRND_RANDOM
#define GRND_RANDOM   0x0002
#endif
#ifndef GRND_INSECURE
#define GRND_INSECURE 0x0004
#endif

#ifdef __cplusplus
extern "C" {
#endif

ssize_t getrandom(void *buf, size_t buflen, unsigned int flags);

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_SYS_RANDOM_H */
