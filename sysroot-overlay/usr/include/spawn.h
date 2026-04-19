/*
 * Part of libsolcompat sysroot — provides POSIX spawn interface for Solaris 7
 *
 * Solaris 7 has no <spawn.h>.  This header provides the full POSIX spawn
 * types, flags, and function declarations backed by libsolcompat's
 * fork/exec implementation.
 */
#ifndef _SOLCOMPAT_SYSROOT_SPAWN_H
#define _SOLCOMPAT_SYSROOT_SPAWN_H

#include <sys/types.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

/* posix_spawn attribute flags */
#ifndef POSIX_SPAWN_RESETIDS
#define POSIX_SPAWN_RESETIDS      0x01
#define POSIX_SPAWN_SETPGROUP     0x02
#define POSIX_SPAWN_SETSIGDEF     0x04
#define POSIX_SPAWN_SETSIGMASK    0x08
#define POSIX_SPAWN_SETSCHEDPARAM 0x10
#define POSIX_SPAWN_SETSCHEDULER  0x20
#endif

/* posix_spawnattr_t — spawn attribute object */
typedef struct {
    short int __flags;
    pid_t     __pgrp;
    sigset_t  __sd;
    sigset_t  __ss;
    int       __sp;
    int       __policy;
    int       __pad[16];
} posix_spawnattr_t;

/* posix_spawn_file_actions_t — file action object */
typedef struct {
    int __allocated;
    int __used;
    void *__actions;
} posix_spawn_file_actions_t;

/* Spawn functions */
int posix_spawn(pid_t *pid, const char *path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *attrp,
                char *const argv[], char *const envp[]);
int posix_spawnp(pid_t *pid, const char *file,
                 const posix_spawn_file_actions_t *file_actions,
                 const posix_spawnattr_t *attrp,
                 char *const argv[], char *const envp[]);

/* Attribute functions */
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

/* Solaris 10+ scheduling attribute accessors. Solaris 7 ignores
 * POSIX_SPAWN_SETSCHEDULER / POSIX_SPAWN_SETSCHEDPARAM flags entirely
 * (scheduling attributes are not applied before exec), but gnulib /
 * Python / libuv consumers still reference the symbols at compile
 * time. struct sched_param comes from <sched.h>. */
struct sched_param;
int posix_spawnattr_setschedpolicy(posix_spawnattr_t *attr, int schedpolicy);
int posix_spawnattr_getschedpolicy(const posix_spawnattr_t *attr, int *schedpolicy);
int posix_spawnattr_setschedparam(posix_spawnattr_t *attr,
                                  const struct sched_param *schedparam);
int posix_spawnattr_getschedparam(const posix_spawnattr_t *attr,
                                  struct sched_param *schedparam);

/* File action functions */
int posix_spawn_file_actions_init(posix_spawn_file_actions_t *fact);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *fact);
int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *fact,
    int fildes, const char *path, int oflag, mode_t mode);
int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *fact, int fildes);
int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *fact,
    int fildes, int newfildes);

#ifdef __cplusplus
}
#endif

/* POSIX 2018 file action extensions. gnulib's execute.c calls
 * addchdir unconditionally; libsolcompat's spawn wrapper applies
 * these before exec. */
int posix_spawn_file_actions_addchdir(posix_spawn_file_actions_t *fact,
    const char *path);
int posix_spawn_file_actions_addfchdir(posix_spawn_file_actions_t *fact,
    int fildes);

#endif /* _SOLCOMPAT_SYSROOT_SPAWN_H */
