/*
 * process.c — Missing process/pipe functions for Solaris 7
 *
 * daemon, err/warn family, pipe2, dup3, mkostemp, posix_spawn
 */

#include <stdarg.h>   /* __gnuc_va_list used by Solaris 7 stdio.h prototypes */
#include <sys/types.h>
#include <sys/stat.h>
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

int
posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *fact,
    int fildes, const char *path, int oflag, mode_t mode)
{
    (void)fact; (void)fildes; (void)path; (void)oflag; (void)mode;
    /* Stub — not fully implemented but satisfies link-time dependency */
    return 0;
}

int
posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *fact, int fildes)
{
    (void)fact; (void)fildes;
    return 0;
}

int
posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *fact,
    int fildes, int newfildes)
{
    (void)fact; (void)fildes; (void)newfildes;
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

/* -----------------------------------------------------------------------
 * execvpe — execute a program, searching PATH from the provided environment.
 *
 * Solaris 7 has execve() (direct path, explicit environment) and execvp()
 * (PATH search, inherits environment) but not execvpe() (PATH search with
 * an explicit environment).  GNU tools increasingly depend on execvpe() to
 * spawn children in a clean, known environment.
 *
 * Algorithm:
 *   1. If filename contains a '/', execute directly via execve() — no search.
 *   2. Otherwise, find PATH in envp (the child's environment, not the
 *      caller's), falling back to the caller's PATH and then the default.
 *   3. Walk each colon-separated directory component, build a candidate
 *      path, and attempt execve().  EACCES is remembered but the search
 *      continues; any other non-ENOENT/ENOTDIR error terminates the loop.
 * ----------------------------------------------------------------------- */
int
execvpe(const char *filename, char *const argv[], char *const envp[])
{
    const char *search_path;
    const char *path_env_value;
    const char *component_start;
    const char *component_end;
    size_t      filename_length;
    size_t      dir_length;
    char        candidate_path[PATH_MAX];
    int         saved_errno;
    int         env_index;

    /* If filename contains a slash, execute directly without PATH search */
    if (strchr(filename, '/') != NULL)
        return execve(filename, argv, envp);

    filename_length = strlen(filename);

    /* Search for PATH in the provided environment (envp), not the caller's */
    path_env_value = NULL;
    if (envp != NULL) {
        for (env_index = 0; envp[env_index] != NULL; env_index++) {
            if (strncmp(envp[env_index], "PATH=", 5) == 0) {
                path_env_value = envp[env_index] + 5;
                break;
            }
        }
    }

    /* Fall back to caller's PATH, then a sensible built-in default */
    if (path_env_value != NULL)
        search_path = path_env_value;
    else
        search_path = getenv("PATH");

    if (search_path == NULL || search_path[0] == '\0')
        search_path = "/usr/bin:/bin";

    saved_errno     = 0;
    component_start = search_path;

    while (component_start != NULL) {
        component_end = strchr(component_start, ':');
        dir_length    = (component_end != NULL)
                        ? (size_t)(component_end - component_start)
                        : strlen(component_start);

        if (dir_length == 0) {
            /* Empty component means the current working directory */
            if (filename_length + 3 <= sizeof(candidate_path)) {
                candidate_path[0] = '.';
                candidate_path[1] = '/';
                memcpy(candidate_path + 2, filename, filename_length + 1);
                execve(candidate_path, argv, envp);
                if (errno == EACCES)
                    saved_errno = EACCES;
                else if (errno != ENOENT && errno != ENOTDIR)
                    break;
            }
        } else if (dir_length + 1 + filename_length + 1 <= sizeof(candidate_path)) {
            memcpy(candidate_path, component_start, dir_length);
            candidate_path[dir_length] = '/';
            memcpy(candidate_path + dir_length + 1, filename, filename_length + 1);
            execve(candidate_path, argv, envp);
            if (errno == EACCES)
                saved_errno = EACCES;
            else if (errno != ENOENT && errno != ENOTDIR)
                break;
        }

        component_start = (component_end != NULL) ? component_end + 1 : NULL;
    }

    /* Report EACCES if that was the only kind of failure encountered */
    if (saved_errno != 0)
        errno = saved_errno;

    return -1;
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
 * We accept attr-level configuration but silently map to stalled
 * (default) behavior. Matches gnulib's fallback expectation.
 * ================================================================== */

int
pthread_mutexattr_setrobust(pthread_mutexattr_t *attr, int robustness)
{
    (void)attr;
    if (robustness != PTHREAD_MUTEX_STALLED && robustness != PTHREAD_MUTEX_ROBUST)
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
 * posix_spawn_file_actions_addchdir / _addfchdir — POSIX 2018. Stubs
 * returning ENOSYS; gnulib's execute.c and other callers fall back to
 * fork+chdir+exec when unavailable. A real implementation would need
 * to extend the __actions list format with a chdir op; defer until a
 * consumer actually breaks on the fallback.
 * ================================================================== */

int
posix_spawn_file_actions_addchdir(posix_spawn_file_actions_t *fact, const char *path)
{
    (void)fact; (void)path;
    return ENOSYS;
}

int
posix_spawn_file_actions_addfchdir(posix_spawn_file_actions_t *fact, int fildes)
{
    (void)fact; (void)fildes;
    return ENOSYS;
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
