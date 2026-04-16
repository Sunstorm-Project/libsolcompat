/*
 * ucred.h — stub for Solaris 7
 *
 * Solaris 10 introduced a <ucred.h> header with a ucred_t opaque type
 * and ucred_get/ucred_free/ucred_size/ucred_geteuid/... API for
 * querying process credentials (including credentials passed over
 * AF_UNIX sockets via SCM_UCRED ancillary data).  Solaris 7 does not
 * ship this header — process credentials are accessible only via
 * getuid/getgid/getpid.
 *
 * glib 2.56+'s gio/gcredentialsprivate.h and similar security-oriented
 * libraries #include <ucred.h> gated on `defined(__sun__)`, assuming
 * any Solaris target satisfies the API.  Without this stub they fail
 * at compile time on Solaris 7 with "ucred.h: No such file or
 * directory".
 *
 * libsolcompat provides the runtime implementations in src/process.c
 * as thin wrappers over getuid/getgid/getpid, matching the single-
 * process semantics (ucred_get(P_MYID) — "my own credentials").
 * Socket-passed credentials (ucred_get(peer_fd) with fd argument) are
 * NOT supported — Solaris 7 libsocket has no SCM_UCRED equivalent.
 * Consumers that call ucred_get on a peer descriptor will receive
 * NULL / errno=ENOSYS.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SOLCOMPAT_UCRED_H
#define _SOLCOMPAT_UCRED_H

#include <sys/types.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque credential handle.  Real Solaris 10+ ucred is far richer (PSR,
 * zone, label, audit info) but consumers use it via accessor functions
 * so the opaque shape is enough for compile-time. */
typedef struct _sol_ucred_s ucred_t;

/* Sentinel pid meaning "current process" (Solaris 10+ <procfs.h>). */
#ifndef P_MYID
#define P_MYID (-1)
#endif

/* Allocation + destruction */
ucred_t *ucred_get(pid_t pid);
void     ucred_free(ucred_t *ucred);
size_t   ucred_size(void);

/* Accessors.  All return (uid_t/gid_t/pid_t)-1 and set errno on failure. */
uid_t    ucred_geteuid(const ucred_t *ucred);
uid_t    ucred_getruid(const ucred_t *ucred);
uid_t    ucred_getsuid(const ucred_t *ucred);
gid_t    ucred_getegid(const ucred_t *ucred);
gid_t    ucred_getrgid(const ucred_t *ucred);
gid_t    ucred_getsgid(const ucred_t *ucred);
pid_t    ucred_getpid(const ucred_t *ucred);
int      ucred_getgroups(const ucred_t *ucred, const gid_t **groups);

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_UCRED_H */
