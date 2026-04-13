/*
 * sys/xattr.h — extended file attributes stub for Solaris 7
 *
 * Solaris 7 has no extended attributes API (added in Solaris 9 as
 * fsattr). Python/coreutils/curl/gettext/glib/vim include this header
 * expecting the Linux/glibc xattr API. libsolcompat's stubs in
 * src/filesystem.c return -1/ENOTSUP; packages that can't live
 * without xattrs should configure-gate on HAVE_SYS_XATTR_H.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SOLCOMPAT_SYS_XATTR_H
#define _SOLCOMPAT_SYS_XATTR_H

#include <sys/types.h>

#ifndef XATTR_CREATE
#define XATTR_CREATE  1
#define XATTR_REPLACE 2
#endif

#ifdef __cplusplus
extern "C" {
#endif

ssize_t getxattr(const char *path, const char *name, void *value, size_t size);
ssize_t lgetxattr(const char *path, const char *name, void *value, size_t size);
ssize_t fgetxattr(int fd, const char *name, void *value, size_t size);

int setxattr(const char *path, const char *name, const void *value, size_t size, int flags);
int lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags);
int fsetxattr(int fd, const char *name, const void *value, size_t size, int flags);

ssize_t listxattr(const char *path, char *list, size_t size);
ssize_t llistxattr(const char *path, char *list, size_t size);
ssize_t flistxattr(int fd, char *list, size_t size);

int removexattr(const char *path, const char *name);
int lremovexattr(const char *path, const char *name);
int fremovexattr(int fd, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_SYS_XATTR_H */
