/*
 * solcompat/at_funcs.h — *at() directory-relative functions
 *
 * Emulated via mutex-protected fchdir dance on Solaris 7.
 * These are safe for single-threaded code and provide
 * serialized access for multi-threaded code.
 */
#ifndef SOLCOMPAT_AT_FUNCS_H
#define SOLCOMPAT_AT_FUNCS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * gnulib and many GNU tools define AT_FDCWD as -3041965
 * (the Solaris 9+ value), NOT -100 (the Linux value).
 * We MUST use the same value or *at() functions break.
 */
#ifndef AT_FDCWD
#define AT_FDCWD             (-3041965)
#endif
/* Also recognize the Linux value in case any code uses it */
#define AT_FDCWD_LINUX       (-100)
#define AT_FDCWD_ALT         AT_FDCWD_LINUX
#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW  0x100
#endif
#ifndef AT_REMOVEDIR
#define AT_REMOVEDIR         0x200
#endif
#ifndef AT_EACCESS
#define AT_EACCESS           0x0010
#endif
#ifndef AT_SYMLINK_FOLLOW
#define AT_SYMLINK_FOLLOW    0x400
#endif

int openat(int dirfd, const char *pathname, int flags, ...);

int mkdirat(int dirfd, const char *pathname, mode_t mode);

int renameat(int olddirfd, const char *oldpath,
             int newdirfd, const char *newpath);

int unlinkat(int dirfd, const char *pathname, int flags);

/* The original bare symbol uses the large-file layout. */
#if defined(__sun) && !defined(_LP64) && (!defined(_FILE_OFFSET_BITS) || _FILE_OFFSET_BITS != 64)
#define fstatat solcompat_fstatat32
#endif
int fstatat(int dirfd, const char *pathname,
            struct stat *buf, int flags);

int fchownat(int dirfd, const char *pathname,
             uid_t owner, gid_t group, int flags);

int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);

ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);

int symlinkat(const char *target, int newdirfd, const char *linkpath);

int linkat(int olddirfd, const char *oldpath,
           int newdirfd, const char *newpath, int flags);

int faccessat(int dirfd, const char *pathname, int mode, int flags);

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_AT_FUNCS_H */
