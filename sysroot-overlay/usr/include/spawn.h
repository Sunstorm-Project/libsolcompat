/*
 * libsolcompat — override <spawn.h>
 *
 * Solaris 7 does not have <spawn.h>. This provides the POSIX spawn
 * interface backed by the fork/exec implementation in libsolcompat.
 */
#ifndef _SOLCOMPAT_OVERRIDE_SPAWN_H
#define _SOLCOMPAT_OVERRIDE_SPAWN_H

#include <sys/types.h>
#include <signal.h>
#include <sched.h>

#include <solcompat/process.h>  /* posix_spawn, posix_spawnp, types */

#endif /* _SOLCOMPAT_OVERRIDE_SPAWN_H */
