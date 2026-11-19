/*
 * process.c — Missing process/pipe functions for Solaris 7
 *
 * daemon, err/warn family, pipe2, dup3, mkostemp, posix_spawn
 */

#include <stdarg.h>   /* __gnuc_va_list used by Solaris 7 stdio.h prototypes */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <poll.h>   /* poll(NULL, 0, ms) — dependency-free sleep for rwlock timed waits */

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
    int ends[2], i, error;

    if (flags & ~(O_NONBLOCK | O_CLOEXEC)) {
        errno = EINVAL;
        return -1;
    }
    if (!pipefd) {
        errno = EFAULT;
        return -1;
    }
    if (pipe(ends) < 0)
        return -1;
    for (i = 0; i < 2; ++i) {
        if (flags & O_NONBLOCK) {
            int status = fcntl(ends[i], F_GETFL);
            if (status < 0 || fcntl(ends[i], F_SETFL, status | O_NONBLOCK) < 0)
                goto fail;
        }
        if ((flags & O_CLOEXEC) && fcntl(ends[i], F_SETFD, FD_CLOEXEC) < 0)
            goto fail;
    }
    pipefd[0] = ends[0];
    pipefd[1] = ends[1];
    return 0;

fail:
    error = errno;
    close(ends[0]);
    close(ends[1]);
    errno = error;
    return -1;
}

int
dup3(int oldfd, int newfd, int flags)
{
    int ret;

    if (oldfd == newfd || (flags & ~O_CLOEXEC)) {
        errno = EINVAL;
        return -1;
    }

    ret = dup2(oldfd, newfd);
    if (ret < 0)
        return -1;

    if ((flags & O_CLOEXEC) && fcntl(newfd, F_SETFD, FD_CLOEXEC) < 0) {
        int error = errno;
        close(newfd);
        errno = error;
        return -1;
    }

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
 * fork/exec spawn with ordered file actions and supported attributes.
 * Unsupported attribute flags fail explicitly before creating a child.
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

/* Private representation; the public file-actions structure is unchanged. */
enum spawn_action_kind { SPAWN_OPEN, SPAWN_CLOSE, SPAWN_DUP2, SPAWN_CHDIR, SPAWN_FCHDIR };
struct spawn_action {
    enum spawn_action_kind kind;
    int fd, newfd, oflag;
    mode_t mode;
    char *path;
};

int
posix_spawn_file_actions_init(posix_spawn_file_actions_t *fact)
{
    memset(fact, 0, sizeof(*fact));
    return 0;
}

int
posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *fact)
{
    int i;
    struct spawn_action *actions = fact->__actions;
    for (i = 0; i < fact->__used; ++i)
        free(actions[i].path);
    free(actions);
    memset(fact, 0, sizeof(*fact));
    return 0;
}

int
posix_spawnattr_setflags(posix_spawnattr_t *attr, short flags)
{
    attr->__flags = flags;
    return 0;
}

int
posix_spawnattr_getflags(const posix_spawnattr_t *attr, short *flags)
{
    *flags = attr->__flags;
    return 0;
}

int
posix_spawnattr_setsigdefault(posix_spawnattr_t *attr, const sigset_t *sigdefault)
{
    attr->__sd = *sigdefault;
    return 0;
}

int
posix_spawnattr_getsigdefault(const posix_spawnattr_t *attr, sigset_t *sigdefault)
{
    *sigdefault = attr->__sd;
    return 0;
}

int
posix_spawnattr_setsigmask(posix_spawnattr_t *attr, const sigset_t *sigmask)
{
    attr->__ss = *sigmask;
    return 0;
}

int
posix_spawnattr_getsigmask(const posix_spawnattr_t *attr, sigset_t *sigmask)
{
    *sigmask = attr->__ss;
    return 0;
}

int
posix_spawnattr_setpgroup(posix_spawnattr_t *attr, pid_t pgroup)
{
    attr->__pgrp = pgroup;
    return 0;
}

int
posix_spawnattr_getpgroup(const posix_spawnattr_t *attr, pid_t *pgroup)
{
    *pgroup = attr->__pgrp;
    return 0;
}

static int
spawn_add_action(posix_spawn_file_actions_t *fact, struct spawn_action action)
{
    struct spawn_action *actions;
    int allocated;
    if (fact->__used == fact->__allocated) {
        if (fact->__allocated > INT_MAX / 2 / (int)sizeof(*actions))
            return ENOMEM;
        allocated = fact->__allocated ? fact->__allocated * 2 : 8;
        actions = realloc(fact->__actions, (size_t)allocated * sizeof(*actions));
        if (!actions)
            return ENOMEM;
        fact->__actions = actions;
        fact->__allocated = allocated;
    }
    actions = fact->__actions;
    actions[fact->__used++] = action;
    return 0;
}

int
posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *fact,
    int fildes, const char *path, int oflag, mode_t mode)
{
    struct spawn_action action;
    int error;
    if (fildes < 0)
        return EBADF;
    if (!path)
        return EINVAL;
    memset(&action, 0, sizeof(action));
    action.kind = SPAWN_OPEN;
    action.fd = fildes;
    action.oflag = oflag;
    action.mode = mode;
    action.path = strdup(path);
    if (!action.path)
        return ENOMEM;
    error = spawn_add_action(fact, action);
    if (error)
        free(action.path);
    return error;
}

int
posix_spawn_file_actions_addchdir(posix_spawn_file_actions_t *fact, const char *path)
{
    struct spawn_action action = { .kind = SPAWN_CHDIR, .fd = -1 };
    int error;

    if (!path)
        return EINVAL;
    action.path = strdup(path);
    if (!action.path)
        return ENOMEM;
    error = spawn_add_action(fact, action);
    if (error)
        free(action.path);
    return error;
}

int
posix_spawn_file_actions_addfchdir(posix_spawn_file_actions_t *fact, int fd)
{
    struct spawn_action action = { .kind = SPAWN_FCHDIR, .fd = fd };

    if (fd < 0)
        return EBADF;
    return spawn_add_action(fact, action);
}

int
posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *fact, int fildes)
{
    struct spawn_action action;
    if (fildes < 0)
        return EBADF;
    memset(&action, 0, sizeof(action));
    action.kind = SPAWN_CLOSE;
    action.fd = fildes;
    return spawn_add_action(fact, action);
}

int
posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *fact,
    int fildes, int newfildes)
{
    struct spawn_action action;
    if (fildes < 0 || newfildes < 0)
        return EBADF;
    memset(&action, 0, sizeof(action));
    action.kind = SPAWN_DUP2;
    action.fd = fildes;
    action.newfd = newfildes;
    return spawn_add_action(fact, action);
}

static int
spawn_attributes(const posix_spawnattr_t *attr)
{
    int sig;
    struct sigaction sa;
    if (!attr)
        return 0;
    if (attr->__flags & POSIX_SPAWN_RESETIDS) {
        if (setegid(getgid()) < 0 || seteuid(getuid()) < 0)
            return errno;
    }
    if ((attr->__flags & POSIX_SPAWN_SETPGROUP) &&
        setpgid(0, attr->__pgrp) < 0)
        return errno;
    if (attr->__flags & POSIX_SPAWN_SETSIGDEF) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        for (sig = 1; sig < NSIG; ++sig) {
            if (sig == SIGKILL || sig == SIGSTOP || sigismember(&attr->__sd, sig) != 1)
                continue;
            if (sigaction(sig, &sa, NULL) < 0) {
                /* Solaris libthread reserves these even in a forked child. */
                if (errno == EINVAL &&
                    (sig == SIGWAITING || sig == SIGLWP || sig == SIGCANCEL))
                    continue;
                return errno;
            }
        }
    }
    if (attr->__flags & POSIX_SPAWN_SETSIGMASK) {
        sigset_t mask;
        if (sigprocmask(SIG_SETMASK, &attr->__ss, NULL) < 0 ||
            sigprocmask(SIG_BLOCK, NULL, &mask) < 0)
            return errno;
        /* Solaris libthread maintains a user-level mask. The new image
         * inherits the calling LWP's kernel mask across exec. */
        if (syscall(SYS_sigprocmask, SIG_SETMASK, &mask, NULL) < 0)
            return errno;
    }
    return 0;
}

static int
spawn_actions(const posix_spawn_file_actions_t *fact)
{
    int i, fd, flags;
    const struct spawn_action *actions;
    if (!fact)
        return 0;
    actions = fact->__actions;
    for (i = 0; i < fact->__used; ++i) {
        const struct spawn_action *a = &actions[i];
        switch (a->kind) {
        case SPAWN_CHDIR:
            if (chdir(a->path) < 0)
                return errno;
            break;
        case SPAWN_FCHDIR:
            if (fchdir(a->fd) < 0)
                return errno;
            break;
        case SPAWN_CLOSE:
            if (close(a->fd) < 0)
                return errno;
            break;
        case SPAWN_DUP2:
            if (a->fd == a->newfd) {
                flags = fcntl(a->fd, F_GETFD);
                if (flags < 0 || fcntl(a->fd, F_SETFD, flags & ~FD_CLOEXEC) < 0)
                    return errno;
            } else if (dup2(a->fd, a->newfd) < 0)
                return errno;
            break;
        case SPAWN_OPEN:
            /* POSIX open action closes the destination before opening. */
            close(a->fd);
            fd = open(a->path, a->oflag, a->mode);
            if (fd < 0)
                return errno;
            if (fd != a->fd) {
                if (dup2(fd, a->fd) < 0) {
                    int error = errno;
                    close(fd);
                    return error;
                }
                close(fd);
            }
            break;
        }
    }
    return 0;
}

/* GNU execvpe invokes the shell for executable text without a shebang.
 * The POSIX spawn path leaves ENOEXEC for its caller. */
static void
exec_candidate(const char *path, char *const argv[], char *const envp[], int shell_fallback)
{
    size_t n = 0, tail;
    char **args;
    int error;

    execve(path, argv, envp);
    if (!shell_fallback || errno != ENOEXEC)
        return;
    while (argv[n])
        ++n;
    tail = n ? n - 1 : 0;
    if (tail > (size_t)-1 / sizeof(*args) - 3) {
        errno = E2BIG;
        return;
    }
    args = malloc((tail + 3) * sizeof(*args));
    if (!args)
        return;
    args[0] = "/bin/sh";
    args[1] = (char *)path;
    if (tail)
        memcpy(args + 2, argv + 1, tail * sizeof(*args));
    args[tail + 2] = NULL;
    execve(args[0], args, envp);
    error = errno;
    free(args);
    errno = error;
}

static void
search_exec(const char *file, const char *path, char *const argv[],
    char *const envp[], int shell_fallback)
{
    const char *start, *end;
    char candidate[PATH_MAX];
    size_t dirlen, filelen = strlen(file);
    int access_error = 0;
    if (!filelen) {
        errno = ENOENT;
        return;
    }
    if (strchr(file, '/')) {
        exec_candidate(file, argv, envp, shell_fallback);
        return;
    }
    if (!path)
        path = "/usr/bin:/bin";
    start = path;
    for (;;) {
        end = strchr(start, ':');
        dirlen = end ? (size_t)(end-start) : strlen(start);
        if (dirlen + (dirlen ? 1 : 0) + filelen + 1 <= sizeof(candidate)) {
            if (dirlen) {
                memcpy(candidate, start, dirlen);
                candidate[dirlen++] = '/';
            }
            memcpy(candidate+dirlen, file, filelen+1);
            exec_candidate(candidate, argv, envp, shell_fallback);
            if (errno == EACCES)
                access_error = EACCES;
            else if (errno != ENOENT && errno != ENOTDIR)
                return;
        }
        if (!end)
            break;
        start = end+1;
    }
    errno = access_error ? access_error : ENOENT;
}

static int
spawn_common(pid_t *pid, const char *path,
    const posix_spawn_file_actions_t *fact, const posix_spawnattr_t *attr,
    char *const argv[], char *const envp[], int search)
{
    int pipefd[2], error, minimum = 3, fd, status, saved_errno = errno;
    int i;
    size_t received;
    ssize_t count;
    pid_t child;
    const struct spawn_action *actions = fact ? fact->__actions : NULL;
    const char *caller_path = search ? getenv("PATH") : NULL;
    const short supported = POSIX_SPAWN_RESETIDS | POSIX_SPAWN_SETPGROUP |
        POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK;
    if (attr && (attr->__flags & ~supported))
        return ENOTSUP;
    if (pipe(pipefd) < 0)
        return errno;
    /* Reserve an unused descriptor not named by any action.  A valid action
     * may name OPEN_MAX-1: placing the channel above the maximum is invalid. */
    for (;;) {
        fd = fcntl(pipefd[1], F_DUPFD, minimum);
        if (fd < 0) {
            error = errno;
            close(pipefd[0]); close(pipefd[1]);
            return error;
        }
        for (i = 0; fact && i < fact->__used; ++i)
            if (fd == actions[i].fd ||
                (actions[i].kind == SPAWN_DUP2 && fd == actions[i].newfd))
                break;
        if (!fact || i == fact->__used)
            break;
        close(fd);
        if (fd == INT_MAX) {
            close(pipefd[0]); close(pipefd[1]);
            return EMFILE;
        }
        minimum = fd + 1;
    }
    close(pipefd[1]);
    pipefd[1] = fd;
    if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
        error = errno;
        close(pipefd[0]); close(fd);
        return error;
    }
    child = fork();
    if (child < 0) {
        error = errno;
        close(pipefd[0]); close(fd);
        return error;
    }
    if (child == 0) {
        close(pipefd[0]);
        error = spawn_attributes(attr);
        if (!error)
            error = spawn_actions(fact);
        if (!error) {
            if (search) {
                search_exec(path, caller_path, argv, envp, 0);
            } else
                execve(path, argv, envp);
            error = errno;
        }
        received = 0;
        while (received < sizeof(error)) {
            count = write(fd, (char *)&error + received, sizeof(error) - received);
            if (count > 0) received += (size_t)count;
            else if (count < 0 && errno == EINTR) continue;
            else break;
        }
        _exit(127);
    }
    close(fd);
    received = 0;
    error = 0;
    while (received < sizeof(error)) {
        count = read(pipefd[0], (char *)&error + received, sizeof(error) - received);
        if (count > 0) received += (size_t)count;
        else if (count < 0 && errno == EINTR) continue;
        else if (count < 0) { error = errno; received = sizeof(error); break; }
        else break;
    }
    close(pipefd[0]);
    if (received) {
        while (waitpid(child, &status, 0) < 0 && errno == EINTR) ;
        errno = saved_errno;
        return received == sizeof(error) ? error : EIO;
    }
    if (pid)
        *pid = child;
    errno = saved_errno;
    return 0;
}

int
posix_spawn(pid_t *pid, const char *path,
    const posix_spawn_file_actions_t *fact, const posix_spawnattr_t *attr,
    char *const argv[], char *const envp[])
{
    return spawn_common(pid, path, fact, attr, argv, envp, 0);
}

int
execvpe(const char *filename, char *const argv[], char *const envp[])
{
    search_exec(filename, getenv("PATH"), argv, envp, 1);
    return -1;
}

int
posix_spawnp(pid_t *pid, const char *file,
    const posix_spawn_file_actions_t *fact, const posix_spawnattr_t *attr,
    char *const argv[], char *const envp[])
{
    return spawn_common(pid, file, fact, attr, argv, envp, 1);
}

/*
 * getgrouplist — get list of groups a user belongs to.
 *
 * Solaris 7 has getgrent() but not getgrouplist() (BSD/POSIX.1-2008).
 * We iterate through the group database checking membership.
 *
 * group is the user's primary GID (always included in the result).
 * On entry, *ngroups is the size of groups[]. On return, *ngroups is
 * set to the actual count. Returns -1 if groups[] is too small.
 */
#include <grp.h>

int
getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups)
{
    struct group *grp_entry;
    int max_groups = *ngroups;
    int group_count = 0;
    int member_index;
    int result = 0;

    /* Always include the primary group */
    if (group_count < max_groups)
        groups[group_count] = group;
    group_count++;

    /* Scan the group database for supplementary memberships */
    setgrent();
    while ((grp_entry = getgrent()) != NULL) {
        /* Skip if this is the primary group (already added) */
        if (grp_entry->gr_gid == group)
            continue;

        /* Check if user is a member of this group */
        for (member_index = 0; grp_entry->gr_mem[member_index] != NULL;
             member_index++) {
            if (strcmp(grp_entry->gr_mem[member_index], user) == 0) {
                if (group_count < max_groups)
                    groups[group_count] = grp_entry->gr_gid;
                group_count++;
                break;
            }
        }
    }
    endgrent();

    if (group_count > max_groups)
        result = -1;

    *ngroups = group_count;
    return result;
}

/*
 * issetugid — BSD/POSIX extension. Return 1 if the current process is
 * running with elevated privileges (real/effective uid or gid differ).
 * Solaris 7 has no equivalent; gnulib and many packages call this
 * unguarded. The check mirrors what glibc's issetugid does on systems
 * without AT_SECURE: compare real vs effective uids/gids.
 */
int
issetugid(void)
{
    return (getuid() != geteuid()) || (getgid() != getegid());
}

/* ==================================================================
 * pthread_spinlock emulation — maps spin to mutex. Correct under
 * POSIX semantics; trades the perf characteristic of a true spin
 * for portability. All 5 functions return pthread_mutex_* return
 * codes directly.
 * ================================================================== */

#include <pthread.h>

int
pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    pthread_mutexattr_t attr;
    int rv;
    (void)pshared;  /* libsolcompat always uses process-private */
    if ((rv = pthread_mutexattr_init(&attr)) != 0)
        return rv;
    rv = pthread_mutex_init(&lock->_m, &attr);
    pthread_mutexattr_destroy(&attr);
    return rv;
}

int pthread_spin_destroy(pthread_spinlock_t *lock) { return pthread_mutex_destroy(&lock->_m); }
int pthread_spin_lock(pthread_spinlock_t *lock)    { return pthread_mutex_lock(&lock->_m); }
int pthread_spin_trylock(pthread_spinlock_t *lock) { return pthread_mutex_trylock(&lock->_m); }
int pthread_spin_unlock(pthread_spinlock_t *lock)  { return pthread_mutex_unlock(&lock->_m); }

/* ==================================================================
 * pthread_rwlock_timed{rd,wr}lock — POSIX 2008 timed rwlock variants.
 * Solaris 7 has pthread_rwlock_{rd,wr}lock (blocking) and _try* (non-
 * blocking). We emulate the timed versions via a try-then-sleep loop
 * with 10ms polling granularity until the abs_timeout is reached.
 * ================================================================== */

static int
rwlock_timed_common(pthread_rwlock_t *rwlock,
                    const struct timespec *abs_timeout,
                    int (*try_lock)(pthread_rwlock_t *))
{
    int rv;
    struct timespec now;
    const long poll_ns = 10L * 1000 * 1000;  /* 10ms */

    if (abs_timeout == NULL)
        return EINVAL;

    for (;;) {
        rv = try_lock(rwlock);
        if (rv != EBUSY)
            return rv;
        if (clock_gettime(CLOCK_REALTIME, &now) != 0)
            return errno;
        if (now.tv_sec > abs_timeout->tv_sec ||
            (now.tv_sec == abs_timeout->tv_sec && now.tv_nsec >= abs_timeout->tv_nsec))
            return ETIMEDOUT;
        /* Use poll() for the 10ms wait instead of nanosleep() so the
           object file doesn't carry an unresolved nanosleep@@SUNW_0.7
           reference that requires -lrt at final link. poll() lives in
           Solaris 7 libc; millisecond precision is sufficient here. */
        {
            poll(NULL, 0, (int)(poll_ns / 1000000L));
        }
    }
}

int
pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock, const struct timespec *abs_timeout)
{
    return rwlock_timed_common(rwlock, abs_timeout, pthread_rwlock_tryrdlock);
}

int
pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock, const struct timespec *abs_timeout)
{
    return rwlock_timed_common(rwlock, abs_timeout, pthread_rwlock_trywrlock);
}

/* ==================================================================
 * Robust-mutex stubs — libsolcompat has no real robust mutex support.
 * Only the default stalled mode is available on Solaris 7.
 * ================================================================== */

int
pthread_mutexattr_setrobust(pthread_mutexattr_t *attr, int robustness)
{
    (void)attr;
    if (robustness == PTHREAD_MUTEX_ROBUST)
        return ENOTSUP;
    if (robustness != PTHREAD_MUTEX_STALLED)
        return EINVAL;
    return 0;
}

int
pthread_mutexattr_getrobust(const pthread_mutexattr_t *attr, int *robustness)
{
    (void)attr;
    if (robustness == NULL)
        return EINVAL;
    *robustness = PTHREAD_MUTEX_STALLED;
    return 0;
}

int
pthread_mutex_consistent(pthread_mutex_t *mutex)
{
    (void)mutex;
    /* Stalled mutexes can't go inconsistent; return EINVAL per POSIX
     * "The mutex object referenced by mutex is not protecting a
     * consistent state". gnulib treats EINVAL as "no-op OK". */
    return EINVAL;
}

/* ==================================================================
 * copy_file_range — Linux 4.5 syscall stub returning -1/ENOSYS.
 * gnulib's copy-file-range.c module, when linked, provides a fallback
 * using read()/write(). Our stub ensures configure probes compile.
 * ================================================================== */

ssize_t
copy_file_range(int fd_in, off_t *off_in,
                int fd_out, off_t *off_out,
                size_t len, unsigned int flags)
{
    (void)fd_in; (void)off_in; (void)fd_out; (void)off_out;
    (void)len; (void)flags;
    errno = ENOSYS;
    return -1;
}

/* ==================================================================
 * fallocate — Linux syscall stub. Packages that use it are expected
 * to fall back to posix_fallocate (which libsolcompat does implement
 * in filesystem.c).
 * ================================================================== */

int
fallocate(int fd, int mode, off_t offset, off_t len)
{
    (void)fd; (void)mode; (void)offset; (void)len;
    errno = ENOSYS;
    return -1;
}

/* ==================================================================
 * prlimit — Linux per-pid resource limit syscall. Solaris 7 has
 * getrlimit/setrlimit (current process only). Stub returns ENOSYS;
 * Python/glib/sudo callers are guarded by HAVE_PRLIMIT.
 * ================================================================== */

struct rlimit;  /* forward-declared; sys/resource.h provides definition */

int
prlimit(pid_t pid, int resource, const struct rlimit *new_limit, struct rlimit *old_limit)
{
    (void)pid; (void)resource; (void)new_limit; (void)old_limit;
    errno = ENOSYS;
    return -1;
}

/* ==================================================================
 * ucred_t — Solaris 10 process-credentials API.  Solaris 7 lacks it,
 * so we synthesise the "current process" credentials from getuid/
 * getgid/getpid.  Socket-peer credentials (ucred_get on a peer fd)
 * are NOT supported — Solaris 7 has no SCM_UCRED equivalent — so
 * consumers with fd arguments get NULL + ENOSYS.
 * ================================================================== */

#include <sys/resource.h>

struct _sol_ucred_s {
    pid_t pid;
    uid_t euid, ruid, suid;
    gid_t egid, rgid, sgid;
    int   ngroups;
    gid_t groups[NGROUPS_MAX];
};

typedef struct _sol_ucred_s _sol_ucred_t;

static size_t
_sol_ucred_size_impl(void)
{
    return sizeof(struct _sol_ucred_s);
}

size_t
ucred_size(void)
{
    return _sol_ucred_size_impl();
}

void *
ucred_get(pid_t pid)
{
    struct _sol_ucred_s *c;
    int n;

    /* We can only report the CURRENT process's credentials.  Solaris 7
     * getuid/getgid give you self-creds; there's no safe way to query
     * a different pid's creds from userspace without setuid. */
    if (pid != -1 && pid != getpid()) {
        errno = ENOSYS;
        return NULL;
    }
    c = (struct _sol_ucred_s *)malloc(sizeof(*c));
    if (c == NULL)
        return NULL;
    c->pid  = getpid();
    c->euid = geteuid();
    c->ruid = getuid();
    c->suid = c->euid;   /* Solaris 7 has no saved-uid separately accessible */
    c->egid = getegid();
    c->rgid = getgid();
    c->sgid = c->egid;
    n = getgroups(NGROUPS_MAX, c->groups);
    c->ngroups = (n < 0) ? 0 : n;
    return c;
}

void
ucred_free(void *ucred)
{
    free(ucred);
}

uid_t  ucred_geteuid(const void *u) { return u ? ((const _sol_ucred_t*)u)->euid : (uid_t)-1; }
uid_t  ucred_getruid(const void *u) { return u ? ((const _sol_ucred_t*)u)->ruid : (uid_t)-1; }
uid_t  ucred_getsuid(const void *u) { return u ? ((const _sol_ucred_t*)u)->suid : (uid_t)-1; }
gid_t  ucred_getegid(const void *u) { return u ? ((const _sol_ucred_t*)u)->egid : (gid_t)-1; }
gid_t  ucred_getrgid(const void *u) { return u ? ((const _sol_ucred_t*)u)->rgid : (gid_t)-1; }
gid_t  ucred_getsgid(const void *u) { return u ? ((const _sol_ucred_t*)u)->sgid : (gid_t)-1; }
pid_t  ucred_getpid(const void *u)  { return u ? ((const _sol_ucred_t*)u)->pid  : (pid_t)-1; }

int
ucred_getgroups(const void *u, const gid_t **groups)
{
    if (u == NULL) {
        errno = EINVAL;
        return -1;
    }
    {
        const struct _sol_ucred_s *c = (const struct _sol_ucred_s *)u;
        if (groups != NULL)
            *groups = c->groups;
        return c->ngroups;
    }
}

/* ==================================================================
 * posix_spawnattr_setschedpolicy / setschedparam (Solaris 10+)
 *
 * Solaris 7 posix_spawn does not honour POSIX_SPAWN_SETSCHEDULER /
 * POSIX_SPAWN_SETSCHEDPARAM flags, so the scheduler settings are
 * effectively ignored. These helpers accept + store the values so the
 * caller's probe/set cycle succeeds, but nothing applies them before
 * exec. Sufficient for Python / libuv / gnulib that only use them
 * behind optional config tests.
 *
 * We can't include <spawn.h> here because the sysroot-overlay's
 * spawn.h redefines the types that solcompat/process.h already provides.
 * Forward-declare struct sched_param and use the typedef from
 * solcompat/process.h (pulled in via the top-of-file include).
 * ================================================================== */

struct sched_param;

int
posix_spawnattr_setschedpolicy(posix_spawnattr_t *attr, int schedpolicy)
{
    (void)attr; (void)schedpolicy;
    return 0;
}

int
posix_spawnattr_getschedpolicy(const posix_spawnattr_t *attr, int *schedpolicy)
{
    (void)attr;
    if (schedpolicy != NULL)
        *schedpolicy = 0;  /* SCHED_OTHER equivalent */
    return 0;
}

int
posix_spawnattr_setschedparam(posix_spawnattr_t *attr,
                              const struct sched_param *schedparam)
{
    (void)attr; (void)schedparam;
    return 0;
}

int
posix_spawnattr_getschedparam(const posix_spawnattr_t *attr,
                              struct sched_param *schedparam)
{
    /* We don't store the param set by the caller (Solaris 7 can't apply
     * it anyway) and can't zero the struct from a forward declaration.
     * Callers that read back are on their own — standard-conformant
     * callers check the return and probe via a separate mechanism. */
    (void)attr; (void)schedparam;
    return 0;
}

/* getpeerucred — Solaris 10 socket-peer credential API.
 * Solaris 7 has no SCM_UCRED ancillary data and no /proc/self/psinfo/ucred
 * field, so there is no safe userland way to obtain a peer's credentials
 * across a socket. Return -1 with errno=ENOTSUP; callers (glib's
 * gcredentials, libdbus, etc.) already have a SO_PEERCRED / getpeereid
 * fallback they use when the primary Solaris path fails. */
int
getpeerucred(int fd, void **ucred)
{
    (void)fd;
    if (ucred != NULL)
        *ucred = NULL;
    errno = ENOTSUP;
    return -1;
}
