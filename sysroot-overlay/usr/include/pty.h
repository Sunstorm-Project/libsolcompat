/*
 * Part of libsolcompat sysroot — provides PTY functions for Solaris 7
 *
 * Solaris 7 has no <pty.h>.  This header provides openpty, forkpty,
 * login_tty, posix_openpt, and cfmakeraw backed by libsolcompat's
 * /dev/ptmx-based implementation.
 */
#ifndef _SOLCOMPAT_SYSROOT_PTY_H
#define _SOLCOMPAT_SYSROOT_PTY_H

#include <termios.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int posix_openpt(int flags);

int openpty(int *amaster, int *aslave, char *name,
            const struct termios *termp,
            const struct winsize *winp);

pid_t forkpty(int *amaster, char *name,
              const struct termios *termp,
              const struct winsize *winp);

int login_tty(int fd);

void cfmakeraw(struct termios *termios_p);

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_SYSROOT_PTY_H */
