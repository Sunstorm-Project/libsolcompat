/*
 * pty.c — PTY functions for Solaris 7
 *
 * Solaris 7 uses the STREAMS-based /dev/ptmx master clone device.
 * ptsname(), grantpt(), unlockpt() are already available.
 * We add: posix_openpt, openpty, forkpty, login_tty, cfmakeraw
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stropts.h>     /* Solaris STREAMS: ioctl I_PUSH */

int
posix_openpt(int flags)
{
    return open("/dev/ptmx", flags);
}

int
openpty(int *amaster, int *aslave, char *name,
        const struct termios *termp,
        const struct winsize *winp)
{
    int master, slave;
    char *slavename;

    master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0)
        return -1;

    if (grantpt(master) < 0 || unlockpt(master) < 0) {
        close(master);
        return -1;
    }

    slavename = ptsname(master);
    if (!slavename) {
        close(master);
        return -1;
    }

    slave = open(slavename, O_RDWR | O_NOCTTY);
    if (slave < 0) {
        close(master);
        return -1;
    }

    /*
     * Push STREAMS modules for terminal semantics.
     * ptem: pseudo-terminal emulation module
     * ldterm: line discipline module
     * ttcompat: V7/4BSD/XENIX compatibility
     */
    ioctl(slave, I_PUSH, "ptem");
    ioctl(slave, I_PUSH, "ldterm");
    ioctl(slave, I_PUSH, "ttcompat");

    if (termp)
        tcsetattr(slave, TCSANOW, termp);
    if (winp)
        ioctl(slave, TIOCSWINSZ, winp);

    if (name)
        strcpy(name, slavename);

    *amaster = master;
    *aslave = slave;
    return 0;
}

int
login_tty(int fd)
{
    /* Create a new session */
    if (setsid() < 0)
        return -1;

    /*
     * Acquire controlling terminal.
     * Solaris doesn't have TIOCSCTTY. Instead, the first open() of a
     * terminal device after setsid() automatically makes it the
     * controlling terminal. Re-open the slave pty to trigger this.
     */
    {
        char *tty = ttyname(fd);
        if (tty) {
            int tmpfd = open(tty, O_RDWR);
            if (tmpfd >= 0)
                close(tmpfd);
        }
    }

    /* Redirect stdio */
    if (dup2(fd, 0) < 0) return -1;
    if (dup2(fd, 1) < 0) return -1;
    if (dup2(fd, 2) < 0) return -1;

    if (fd > 2)
        close(fd);

    return 0;
}

pid_t
forkpty(int *amaster, char *name,
        const struct termios *termp,
        const struct winsize *winp)
{
    int master, slave;
    pid_t pid;

    if (openpty(&master, &slave, name, termp, winp) < 0)
        return -1;

    pid = fork();
    if (pid < 0) {
        close(master);
        close(slave);
        return -1;
    }

    if (pid == 0) {
        /* Child */
        close(master);
        if (login_tty(slave) < 0)
            _exit(1);
        return 0;
    }

    /* Parent */
    close(slave);
    *amaster = master;
    return pid;
}

void
cfmakeraw(struct termios *t)
{
    t->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                     INLCR | IGNCR | ICRNL | IXON);
    t->c_oflag &= ~OPOST;
    t->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t->c_cflag &= ~(CSIZE | PARENB);
    t->c_cflag |= CS8;
    t->c_cc[VMIN] = 1;
    t->c_cc[VTIME] = 0;
}
