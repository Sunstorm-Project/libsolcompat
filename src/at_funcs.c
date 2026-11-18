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
#ifndef O_DIRECTORY
#define O_DIRECTORY 0x1000000
#endif
#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 0x100
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

    {
        int error = errno;
        at_leave(saved);
        errno = error;
    }
    return result;
}

int
mkdirat(int dirfd, const char *pathname, mode_t mode)
{
    int saved, result;

    saved = at_enter(dirfd, pathname);
    if (saved < 0)
        return -1;

    result = mkdir(pathname, mode);

    at_leave(saved);
    return result;
}

int
renameat(int olddirfd, const char *oldpath,
         int newdirfd, const char *newpath)
{
    /*
     * This is harder — we need both paths resolved.
     * Strategy: resolve oldpath first, then chdir for newpath.
     * If both are relative to different dirfds, we need full paths.
     */
    char old_full[1024], new_full[1024];
    int result;

    /* Simple case: both AT_FDCWD */
    if (IS_AT_FDCWD(olddirfd) && IS_AT_FDCWD(newdirfd))
        return rename(oldpath, newpath);

    pthread_mutex_lock(&at_mutex);

    /* Resolve old path */
    if (oldpath[0] == '/' || IS_AT_FDCWD(olddirfd)) {
        strlcpy(old_full, oldpath, sizeof(old_full));
    } else {
        char dirpath[1024];
        if (resolve_dirfd_path(olddirfd, dirpath, sizeof(dirpath)) < 0) {
            pthread_mutex_unlock(&at_mutex);
            return -1;
        }
        solcompat_snprintf(old_full, sizeof(old_full), "%s/%s", dirpath, oldpath);
    }

    /* Resolve new path */
    if (newpath[0] == '/' || IS_AT_FDCWD(newdirfd)) {
        strlcpy(new_full, newpath, sizeof(new_full));
    } else {
        char dirpath[1024];
        if (resolve_dirfd_path(newdirfd, dirpath, sizeof(dirpath)) < 0) {
            pthread_mutex_unlock(&at_mutex);
            return -1;
        }
        solcompat_snprintf(new_full, sizeof(new_full), "%s/%s", dirpath, newpath);
    }

    result = rename(old_full, new_full);
    pthread_mutex_unlock(&at_mutex);

    return result;
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

int
fstatat(int dirfd, const char *pathname, struct stat *buf, int flags)
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
        result = lstat(pathname, buf);
    else
        result = stat(pathname, buf);

    at_leave(saved);
    return result;
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

int
linkat(int olddirfd, const char *oldpath,
       int newdirfd, const char *newpath, int flags)
{
    char old_full[1024], new_full[1024];
    int result;

    (void)flags;

    pthread_mutex_lock(&at_mutex);

    /* Resolve old path */
    if (oldpath[0] == '/' || IS_AT_FDCWD(olddirfd)) {
        strlcpy(old_full, oldpath, sizeof(old_full));
    } else {
        char dirpath[1024];
        if (resolve_dirfd_path(olddirfd, dirpath, sizeof(dirpath)) < 0) {
            pthread_mutex_unlock(&at_mutex);
            return -1;
        }
        solcompat_snprintf(old_full, sizeof(old_full), "%s/%s", dirpath, oldpath);
    }

    /* Resolve new path */
    if (newpath[0] == '/' || IS_AT_FDCWD(newdirfd)) {
        strlcpy(new_full, newpath, sizeof(new_full));
    } else {
        char dirpath[1024];
        if (resolve_dirfd_path(newdirfd, dirpath, sizeof(dirpath)) < 0) {
            pthread_mutex_unlock(&at_mutex);
            return -1;
        }
        solcompat_snprintf(new_full, sizeof(new_full), "%s/%s", dirpath, newpath);
    }

    result = link(old_full, new_full);
    pthread_mutex_unlock(&at_mutex);

    return result;
}

int
faccessat(int dirfd, const char *pathname, int mode, int flags)
{
    int saved, result;
    (void)flags;  /* AT_EACCESS not supportable */

    saved = at_enter(dirfd, pathname);
    if (saved < 0)
        return -1;

    result = access(pathname, mode);

    at_leave(saved);
    return result;
}
