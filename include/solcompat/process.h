/*
 * solcompat/process.h -- Missing process/pipe functions
 *
 * daemon, posix_spawn, pipe2, dup3
 *
 * NOTE: err/warn family intentionally NOT declared here to avoid
 * conflicts with packages that use 'warn' or 'err' as variable names
 * (e.g. coreutils, gawk). Use <solcompat/err.h> if needed.
 */
#ifndef SOLCOMPAT_PROCESS_H
#define SOLCOMPAT_PROCESS_H

#include <sys/types.h>
#include <signal.h>

/* O_CLOEXEC doesn't exist on Solaris 7 -- define a placeholder
 * value so bit-test code compiles; pipe2/dup3/mkostemp implement
 * the semantics via fcntl(F_SETFD, FD_CLOEXEC). */
#ifndef O_CLOEXEC
#define O_CLOEXEC 0x800000
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_DAEMON
int daemon(int nochdir, int noclose);
#endif

#ifndef HAVE_PIPE2
int pipe2(int pipefd[2], int flags);
#endif

#ifndef HAVE_DUP3
int dup3(int oldfd, int newfd, int flags);
#endif

#ifndef HAVE_MKOSTEMP
int mkostemp(char *tmpl, int flags);
#endif

/*
 * execvpe — execute a program, searching PATH from the provided environment.
 *
 * Like execvp() but uses envp for the child's environment AND searches PATH
 * within envp (not the caller's PATH).  Solaris 7 has execve() and execvp()
 * but not execvpe().
 */
#ifndef HAVE_EXECVPE
int execvpe(const char *filename, char *const argv[], char *const envp[]);
#endif

/* posix_spawn attribute flags -- always provide these constants
 * even when HAVE_POSIX_SPAWN is defined (libsolcompat provides the
 * functions but Solaris 7 system headers lack the flag constants). */
#ifndef POSIX_SPAWN_RESETIDS
#define POSIX_SPAWN_RESETIDS      0x01
#define POSIX_SPAWN_SETPGROUP     0x02
#define POSIX_SPAWN_SETSIGDEF     0x04
#define POSIX_SPAWN_SETSIGMASK    0x08
#define POSIX_SPAWN_SETSCHEDPARAM 0x10
#define POSIX_SPAWN_SETSCHEDULER  0x20
#endif

/* posix_spawn minimal interface */
#ifndef HAVE_POSIX_SPAWN

typedef struct {
    short int __flags;
    pid_t     __pgrp;
    sigset_t  __sd;
    sigset_t  __ss;
    int       __sp;
    int       __policy;
    int       __pad[16];
} posix_spawnattr_t;

typedef struct {
    int __allocated;
    int __used;
    void *__actions;
} posix_spawn_file_actions_t;

int posix_spawn(pid_t *pid, const char *path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *attrp,
                char *const argv[], char *const envp[]);
int posix_spawnp(pid_t *pid, const char *file,
                 const posix_spawn_file_actions_t *file_actions,
                 const posix_spawnattr_t *attrp,
                 char *const argv[], char *const envp[]);
int posix_spawnattr_init(posix_spawnattr_t *attr);
int posix_spawnattr_destroy(posix_spawnattr_t *attr);
int posix_spawnattr_setflags(posix_spawnattr_t *attr, short flags);
int posix_spawnattr_getflags(const posix_spawnattr_t *attr, short *flags);
int posix_spawnattr_setsigdefault(posix_spawnattr_t *attr, const sigset_t *sigdefault);
int posix_spawnattr_getsigdefault(const posix_spawnattr_t *attr, sigset_t *sigdefault);
int posix_spawnattr_setsigmask(posix_spawnattr_t *attr, const sigset_t *sigmask);
int posix_spawnattr_getsigmask(const posix_spawnattr_t *attr, sigset_t *sigmask);
int posix_spawnattr_setpgroup(posix_spawnattr_t *attr, pid_t pgroup);
int posix_spawnattr_getpgroup(const posix_spawnattr_t *attr, pid_t *pgroup);
int posix_spawn_file_actions_init(posix_spawn_file_actions_t *fact);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *fact);
int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *fact,
    int fildes, const char *path, int oflag, mode_t mode);
int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *fact, int fildes);
int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *fact,
    int fildes, int newfildes);
#endif

/*
 * getgrouplist — enumerate a user's group memberships.
 * BSD/POSIX.1-2008 function, not in Solaris 7.
 */
#ifndef HAVE_GETGROUPLIST
int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups);
#endif

/*
 * sem_timedwait — wait on semaphore with timeout.
 * POSIX.1-2001 function, not in Solaris 7 (which only has sem_wait/sem_trywait).
 */
#include <semaphore.h>
#ifndef HAVE_SEM_TIMEDWAIT
int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_PROCESS_H */
