/*
 * solcompat/process.h — Missing process/pipe functions
 *
 * daemon, err/warn family, posix_spawn, pipe2, dup3
 */
#ifndef SOLCOMPAT_PROCESS_H
#define SOLCOMPAT_PROCESS_H

#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

/* O_CLOEXEC doesn't exist on Solaris 7 — define a placeholder
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

#ifndef HAVE_ERR
void err(int eval, const char *fmt, ...);
void errx(int eval, const char *fmt, ...);
void warn(const char *fmt, ...);
void warnx(const char *fmt, ...);
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
int posix_spawn_file_actions_init(posix_spawn_file_actions_t *fact);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *fact);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_PROCESS_H */
