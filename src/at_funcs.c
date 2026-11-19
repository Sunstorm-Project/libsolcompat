/*
 * at_funcs.c — *at() directory-relative function emulation
 *
 * Solaris 7 has no *at() functions (openat, mkdirat, etc.).
 * We emulate them with a mutex-protected fchdir dance:
 *   1. Lock mutex
 *   2. Save current directory (fchdir from open("."))
 *   3. fchdir(dirfd)
 *   4. Perform the operation with the relative path
 *   5. fchdir back to saved directory
 *   6. Unlock mutex
 *
 * This is safe for single-threaded code.  Multi-threaded code gets
 * serialized through the mutex, which is correct but slow.
 */

/* Must be defined BEFORE any includes so that stat/lstat/fstat become
 * stat64/lstat64/fstat64 and struct stat becomes struct stat64.
 * All callers (GNU coreutils, tar, etc.) are compiled with this flag,
 * so their struct stat is actually struct stat64.  We must match.  */
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <pthread.h>

/* Forward declarations for our own functions */
extern int solcompat_snprintf(char *, size_t, const char *, ...);
extern size_t strlcpy(char *, const char *, size_t);

/*
 * AT_FDCWD must match the value used by gnulib in GNU coreutils,
 * tar, grep, etc. — which is -3041965 (the Solaris 9+ value).
 * We also accept -100 (the Linux value) at runtime.
 */
#ifndef AT_FDCWD
#define AT_FDCWD (-3041965)
#endif
#define AT_FDCWD_LINUX (-100)
/* Public Solaris 7 sysroot ABI, also needed with original system headers. */
#ifndef O_CLOEXEC
#define O_CLOEXEC 0x800000
#endif
#ifndef O_DIRECTORY
#define O_DIRECTORY 0x1000000
#endif
#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 0x100
#endif
#ifndef AT_EACCESS
#define AT_EACCESS 0x0010
#endif
#ifndef AT_REMOVEDIR
#define AT_REMOVEDIR 0x200
#endif

/* Check if dirfd represents "current working directory" */
#define IS_AT_FDCWD(fd) ((fd) == AT_FDCWD || (fd) == AT_FDCWD_LINUX)

static pthread_mutex_t at_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Enter the directory referenced by dirfd, saving the current directory.
 * Returns a file descriptor for the saved directory, or -1 on error.
 * If dirfd is AT_FDCWD or the path is absolute, saves cwd but doesn't chdir.
 */
static int
at_enter(int dirfd, const char *path)
{
    int saved_fd;

    pthread_mutex_lock(&at_mutex);

    saved_fd = open(".", O_RDONLY);
    if (saved_fd < 0) {
        pthread_mutex_unlock(&at_mutex);
        return -1;
    }

    if (path && path[0] == '/') {
        /* Absolute path — no need to chdir */
        return saved_fd;
    }

    if (!IS_AT_FDCWD(dirfd)) {
        if (fchdir(dirfd) < 0) {
            close(saved_fd);
            pthread_mutex_unlock(&at_mutex);
            return -1;
        }
    }

    return saved_fd;
}

static void
at_leave(int saved_fd)
{
    if (saved_fd >= 0) {
        fchdir(saved_fd);
        close(saved_fd);
    }
    pthread_mutex_unlock(&at_mutex);
}

#include "dirfd_internal.h"

int solcompat_with_dirfd(int fd, const char *path,
                        int (*operation)(const char *, void *), void *argument)
{
    if (!path) {
        errno = EFAULT;
        return -1;
    }
    int saved = at_enter(fd, path);
    if (saved < 0)
        return -1;
    int result = operation(path, argument);
    int error = errno;
    at_leave(saved);
    errno = error;
    return result;
}

/*
 * Resolve a dirfd to a directory path string.
 * Solaris 7 /proc/self/fd/N entries are NOT symlinks (readlink fails),
 * so we use fchdir + getcwd instead.
 * Returns 0 on success, -1 on error.
 * NOTE: Does its own open(".")/fchdir dance internally — caller must
 *       NOT hold at_mutex (or must be prepared for nested open/fchdir).
 */
static int
resolve_dirfd_path(int dirfd, char *buf, size_t bufsiz)
{
    int saved;
    char *p;

    saved = open(".", O_RDONLY);
    if (saved < 0)
        return -1;

    if (fchdir(dirfd) < 0) {
        close(saved);
        return -1;
    }

    p = getcwd(buf, bufsiz);

    fchdir(saved);
    close(saved);

    return (p != NULL) ? 0 : -1;
}

int
openat(int dirfd, const char *pathname, int flags, ...)
{
    int saved, result;
    mode_t mode = 0;

    int cloexec = flags & O_CLOEXEC;
    flags &= ~O_CLOEXEC;

    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = (mode_t)va_arg(ap, int);
        va_end(ap);
    }

    saved = at_enter(dirfd, pathname);
    if (saved < 0)
        return -1;

    if (flags & O_DIRECTORY) {
        struct stat st;
        /* Solaris 7 ignores the compatibility bit. Check before open so a
         * FIFO cannot block and an ordinary file cannot be truncated. */
        if (flags & O_CREAT) {
            errno = EINVAL;
            result = -1;
        } else if (stat(pathname, &st) < 0) {
            result = -1;
        } else if (!S_ISDIR(st.st_mode)) {
            errno = ENOTDIR;
            result = -1;
        } else if ((flags & O_ACCMODE) != O_RDONLY || (flags & O_TRUNC)) {
            errno = EISDIR;
            result = -1;
        } else {
            result = open(pathname, (flags & ~O_DIRECTORY) | O_NONBLOCK);
            if (result >= 0) {
                int error = 0;
                if (fstat(result, &st) < 0)
                    error = errno;
                else if (!S_ISDIR(st.st_mode))
                    error = ENOTDIR;
                else if (!(flags & O_NONBLOCK)) {
                    int status = fcntl(result, F_GETFL);
                    if (status < 0 || fcntl(result, F_SETFL, status & ~O_NONBLOCK) < 0)
                        error = errno;
                }
                if (error) {
                    close(result);
                    result = -1;
                    errno = error;
                }
            }
        }
    } else {
        result = open(pathname, flags, mode);
    }

    if (result >= 0 && cloexec && fcntl(result, F_SETFD, FD_CLOEXEC) < 0) {
        int error = errno;
        close(result);
        result = -1;
        errno = error;
    }

    {
        int error = errno;
        at_leave(saved);
        errno = error;
    }
    return result;
}

static int make_directory(const char *path, void *arg)
{
    return mkdir(path, *(mode_t *)arg);
}

int mkdirat(int fd, const char *path, mode_t mode)
{
    return solcompat_with_dirfd(fd, path, make_directory, &mode);
}

static int make_fifo(const char *path, void *arg)
{
    return mkfifo(path, *(mode_t *)arg);
}

int mkfifoat(int fd, const char *path, mode_t mode)
{
    return solcompat_with_dirfd(fd, path, make_fifo, &mode);
}

struct node_args { mode_t mode; dev_t device; };

static int make_node(const char *path, void *arg)
{
    struct node_args *node = arg;
    return mknod(path, node->mode, node->device);
}

int mknodat(int fd, const char *path, mode_t mode, dev_t device)
{
    struct node_args node = {mode, device};
    return solcompat_with_dirfd(fd, path, make_node, &node);
}

/* Caller holds at_mutex while resolving both operands. Never operate on a
 * truncated prefix if a path cannot be represented by Solaris. */
static const char *at_operand(int fd, const char *path, char *buf, size_t size)
{
    char directory[1024];
    if (!path) {
        errno = EFAULT;
        return NULL;
    }
    if (!*path) {
        errno = ENOENT;
        return NULL;
    }
    if (*path == '/' || IS_AT_FDCWD(fd))
        return path;
    if (resolve_dirfd_path(fd, directory, sizeof(directory)) < 0)
        return NULL;
    int n = solcompat_snprintf(buf, size, "%s/%s", directory, path);
    if (n < 0 || (size_t)n >= size) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    return buf;
}

static int two_paths(int oldfd, const char *oldpath, int newfd, const char *newpath,
                     int (*operation)(const char *, const char *))
{
    char oldbuf[1024], newbuf[1024];
    int result = -1;
    pthread_mutex_lock(&at_mutex);
    oldpath = at_operand(oldfd, oldpath, oldbuf, sizeof(oldbuf));
    if (oldpath) {
        newpath = at_operand(newfd, newpath, newbuf, sizeof(newbuf));
        if (newpath)
            result = operation(oldpath, newpath);
    }
    int error = errno;
    pthread_mutex_unlock(&at_mutex);
    errno = error;
    return result;
}

int renameat(int oldfd, const char *oldpath, int newfd, const char *newpath)
{
    return two_paths(oldfd, oldpath, newfd, newpath, rename);
}

int
unlinkat(int dirfd, const char *pathname, int flags)
{
    int saved, result;

    if (flags & ~AT_REMOVEDIR) {
        errno = EINVAL;
        return -1;
    }

    saved = at_enter(dirfd, pathname);
    if (saved < 0)
        return -1;

    if (flags & AT_REMOVEDIR)
        result = rmdir(pathname);
    else
        result = unlink(pathname);

    at_leave(saved);
    return result;
}

#include "statat_internal.h"

static int
stat_large(const char *pathname, void *buf, int flags)
{
    return flags & AT_SYMLINK_NOFOLLOW ? lstat(pathname, buf) : stat(pathname, buf);
}

int
solcompat_statat_with(int dirfd, const char *pathname, void *buf, int flags,
                      solcompat_stat_fn query)
{
    int saved, result, error;
    if (flags & ~AT_SYMLINK_NOFOLLOW) {
        errno = EINVAL;
        return -1;
    }
    saved = at_enter(dirfd, pathname);
    if (saved < 0)
        return -1;
    result = query(pathname, buf, flags);
    error = errno;
    at_leave(saved);
    errno = error;
    return result;
}

int
fstatat(int dirfd, const char *pathname, struct stat *buf, int flags)
{
    return solcompat_statat_with(dirfd, pathname, buf, flags, stat_large);
}

int
fchownat(int dirfd, const char *pathname,
          uid_t owner, gid_t group, int flags)
{
    int saved, result;

    if (flags & ~AT_SYMLINK_NOFOLLOW) {
        errno = EINVAL;
        return -1;
    }

    saved = at_enter(dirfd, pathname);
    if (saved < 0)
        return -1;

    if (flags & AT_SYMLINK_NOFOLLOW)
        result = lchown(pathname, owner, group);
    else
        result = chown(pathname, owner, group);

    at_leave(saved);
    return result;
}

int
fchmodat(int dirfd, const char *pathname, mode_t mode, int flags)
{
    int saved, result;

    if (flags & ~AT_SYMLINK_NOFOLLOW) {
        errno = EINVAL;
        return -1;
    }
    if (flags & AT_SYMLINK_NOFOLLOW) {
        /* Solaris 7 chmod follows links and cannot apply this constraint. */
        errno = ENOTSUP;
        return -1;
    }

    saved = at_enter(dirfd, pathname);
    if (saved < 0)
        return -1;

    result = chmod(pathname, mode);

    at_leave(saved);
    return result;
}

ssize_t
readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz)
{
    int saved;
    ssize_t result;

    saved = at_enter(dirfd, pathname);
    if (saved < 0)
        return -1;

    result = readlink(pathname, buf, bufsiz);

    at_leave(saved);
    return result;
}

int
symlinkat(const char *target, int newdirfd, const char *linkpath)
{
    int saved, result;

    saved = at_enter(newdirfd, linkpath);
    if (saved < 0)
        return -1;

    result = symlink(target, linkpath);

    at_leave(saved);
    return result;
}

int linkat(int oldfd, const char *oldpath, int newfd, const char *newpath, int flags)
{
    if (flags & ~AT_SYMLINK_FOLLOW) {
        errno = EINVAL;
        return -1;
    }
    if (flags & AT_SYMLINK_FOLLOW) {
        errno = ENOTSUP;
        return -1;
    }
    return two_paths(oldfd, oldpath, newfd, newpath, link);
}

int
faccessat(int dirfd, const char *pathname, int mode, int flags)
{
    int saved, result;
    if (flags & ~(AT_EACCESS | AT_SYMLINK_NOFOLLOW)) {
        errno = EINVAL;
        return -1;
    }
    /* access() uses the real IDs and follows the final symlink. */
    if ((flags & AT_SYMLINK_NOFOLLOW) ||
        ((flags & AT_EACCESS) &&
         (getuid() != geteuid() || getgid() != getegid()))) {
        errno = ENOTSUP;
        return -1;
    }

    saved = at_enter(dirfd, pathname);
    if (saved < 0)
        return -1;

    result = access(pathname, mode);

    at_leave(saved);
    return result;
}
