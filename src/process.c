/*
 * process.c — Missing process/pipe functions for Solaris 7
 *
 * daemon, err/warn family, pipe2, dup3, mkostemp, posix_spawn
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>

#include "solcompat/process.h"

extern int solcompat_snprintf(char *, size_t, const char *, ...);

/* Forward declaration — defined below */
static const char *getprogname(void);

int
daemon(int nochdir, int noclose)
{
    pid_t pid;
    int fd;

    pid = fork();
    if (pid < 0)
        return -1;
    if (pid > 0)
        _exit(0);  /* Parent exits */

    /* Child becomes session leader */
    if (setsid() < 0)
        return -1;

    if (!nochdir)
        chdir("/");

    if (!noclose) {
        fd = open("/dev/null", O_RDWR);
        if (fd >= 0) {
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            if (fd > 2)
                close(fd);
        }
    }

    return 0;
}

void
err(int eval, const char *fmt, ...)
{
    va_list ap;
    int saved_errno = errno;

    fprintf(stderr, "%s: ", getprogname() ? getprogname() : "");
    if (fmt) {
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, ": ");
    }
    fprintf(stderr, "%s\n", strerror(saved_errno));
    exit(eval);
}

void
errx(int eval, const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: ", getprogname() ? getprogname() : "");
    if (fmt) {
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    fprintf(stderr, "\n");
    exit(eval);
}

void
warn(const char *fmt, ...)
{
    va_list ap;
    int saved_errno = errno;

    fprintf(stderr, "%s: ", getprogname() ? getprogname() : "");
    if (fmt) {
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, ": ");
    }
    fprintf(stderr, "%s\n", strerror(saved_errno));
}

void
warnx(const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: ", getprogname() ? getprogname() : "");
    if (fmt) {
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    fprintf(stderr, "\n");
}

/*
 * getprogname — Solaris 7 doesn't have it, but we can use
 * getexecname() which is available since Solaris 2.4
 */
static const char *
getprogname(void)
{
    extern const char *getexecname(void);
    const char *p = getexecname();
    const char *slash;

    if (!p)
        return "unknown";
    slash = strrchr(p, '/');
    return slash ? slash + 1 : p;
}

int
pipe2(int pipefd[2], int flags)
{
    if (pipe(pipefd) < 0)
        return -1;

    if (flags & O_NONBLOCK) {
        fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);
        fcntl(pipefd[1], F_SETFL, fcntl(pipefd[1], F_GETFL) | O_NONBLOCK);
    }
    if (flags & O_CLOEXEC) {
        fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
        fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);
    }

    return 0;
}

int
dup3(int oldfd, int newfd, int flags)
{
    int ret;

    if (oldfd == newfd) {
        errno = EINVAL;
        return -1;
    }

    ret = dup2(oldfd, newfd);
    if (ret < 0)
        return -1;

    if (flags & O_CLOEXEC)
        fcntl(newfd, F_SETFD, FD_CLOEXEC);

    return ret;
}

int
mkostemp(char *tmpl, int flags)
{
    int fd = mkstemp(tmpl);
    if (fd < 0)
        return -1;

    if (flags & O_CLOEXEC)
        fcntl(fd, F_SETFD, FD_CLOEXEC);
    if (flags & O_APPEND)
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_APPEND);

    return fd;
}

/*
 * Minimal posix_spawn implementation using fork/exec.
 * This doesn't handle all file_actions or spawn attributes,
 * but covers the common case.
 */

/* undef posix_spawn macros if present, so we can define the actual functions */
#undef posix_spawn
#undef posix_spawnp

int
posix_spawnattr_init(posix_spawnattr_t *attr)
{
    memset(attr, 0, sizeof(*attr));
    return 0;
}

int
posix_spawnattr_destroy(posix_spawnattr_t *attr)
{
    (void)attr;
    return 0;
}

int
posix_spawn_file_actions_init(posix_spawn_file_actions_t *fact)
{
    memset(fact, 0, sizeof(*fact));
    return 0;
}

int
posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *fact)
{
    if (fact->__actions)
        free(fact->__actions);
    memset(fact, 0, sizeof(*fact));
    return 0;
}

int
posix_spawn(pid_t *pid, const char *path,
            const posix_spawn_file_actions_t *file_actions,
            const posix_spawnattr_t *attrp,
            char *const argv[], char *const envp[])
{
    pid_t child;

    (void)file_actions;  /* Not fully implemented */
    (void)attrp;

    child = fork();
    if (child < 0)
        return errno;

    if (child == 0) {
        /* Child */
        execve(path, argv, envp);
        _exit(127);
    }

    /* Parent */
    if (pid)
        *pid = child;
    return 0;
}

int
posix_spawnp(pid_t *pid, const char *file,
             const posix_spawn_file_actions_t *file_actions,
             const posix_spawnattr_t *attrp,
             char *const argv[], char *const envp[])
{
    pid_t child;

    (void)file_actions;
    (void)attrp;

    child = fork();
    if (child < 0)
        return errno;

    if (child == 0) {
        /* Child — use PATH lookup.  environ is ignored in execvp,
         * but we set it before execvp */
        if (envp) {
            extern char **environ;
            environ = (char **)envp;
        }
        execvp(file, argv);
        _exit(127);
    }

    if (pid)
        *pid = child;
    return 0;
}
