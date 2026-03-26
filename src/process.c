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
#include <limits.h>

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
