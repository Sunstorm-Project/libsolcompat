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

/*
 * getrandom() prototype is INTENTIONALLY OMITTED here.
 *
 * bash-5.3's lib/sh/random.c declares its own `static int getrandom(...)`
 * as a fallback when autoconf doesn't detect the libc version. If we
 * expose an extern prototype via this header, bash's static declaration
 * collides with it and fails compilation ("static declaration of
 * 'getrandom' follows non-static declaration"). Leaving the prototype
 * out lets bash's static live in its own TU scope.
 *
 * Packages that explicitly want libsolcompat's getrandom() can either:
 *   - declare it themselves (`extern ssize_t getrandom(void *, size_t,
 *     unsigned int);`)
 *   - include <solcompat/random.h> from libsolcompat-dev
 *
 * Autoconf-style probes that link-test `getrandom` still succeed since
 * libsolcompat.so.1 exports the symbol — they just won't find a
 * prototype in <sys/random.h>, which is the Linux-style discoverability
 * tradeoff we accept here.
 */

#endif /* _SOLCOMPAT_SYS_RANDOM_H */
