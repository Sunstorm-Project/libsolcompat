/*
 * solcompat/pty.h — PTY functions
 *
 * posix_openpt, openpty, forkpty, login_tty, cfmakeraw
 */
#ifndef SOLCOMPAT_PTY_H
#define SOLCOMPAT_PTY_H

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

int tcgetwinsize(int fd, struct winsize *size);
int tcsetwinsize(int fd, const struct winsize *size);

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_PTY_H */
