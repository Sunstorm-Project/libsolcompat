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

#ifndef AT_FDCWD
#define AT_FDCWD (-100)
#endif
#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 0x100
#endif
#ifndef AT_REMOVEDIR
#define AT_REMOVEDIR 0x200
#endif

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

    if (dirfd != AT_FDCWD) {
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

    result = open(pathname, flags, mode);

    at_leave(saved);
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
    if (olddirfd == AT_FDCWD && newdirfd == AT_FDCWD)
        return rename(oldpath, newpath);

    /* Resolve old path */
    if (oldpath[0] == '/' || olddirfd == AT_FDCWD) {
        strlcpy(old_full, oldpath, sizeof(old_full));
    } else {
        char procfd[64], dirpath[1024];
        ssize_t len;
        solcompat_snprintf(procfd, sizeof(procfd), "/proc/self/fd/%d", olddirfd);
        len = readlink(procfd, dirpath, sizeof(dirpath) - 1);
        if (len < 0) return -1;
        dirpath[len] = '\0';
        solcompat_snprintf(old_full, sizeof(old_full), "%s/%s", dirpath, oldpath);
    }

    /* Resolve new path */
    if (newpath[0] == '/' || newdirfd == AT_FDCWD) {
        strlcpy(new_full, newpath, sizeof(new_full));
    } else {
        char procfd[64], dirpath[1024];
        ssize_t len;
        solcompat_snprintf(procfd, sizeof(procfd), "/proc/self/fd/%d", newdirfd);
        len = readlink(procfd, dirpath, sizeof(dirpath) - 1);
        if (len < 0) return -1;
        dirpath[len] = '\0';
        solcompat_snprintf(new_full, sizeof(new_full), "%s/%s", dirpath, newpath);
    }

    pthread_mutex_lock(&at_mutex);
    result = rename(old_full, new_full);
    pthread_mutex_unlock(&at_mutex);

    return result;
}

int
unlinkat(int dirfd, const char *pathname, int flags)
{
    int saved, result;

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
    (void)flags;  /* AT_SYMLINK_NOFOLLOW not possible with chmod on Solaris 7 */

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

    /* Resolve old path */
    if (oldpath[0] == '/' || olddirfd == AT_FDCWD) {
        strlcpy(old_full, oldpath, sizeof(old_full));
    } else {
        char procfd[64], dirpath[1024];
        ssize_t len;
        solcompat_snprintf(procfd, sizeof(procfd), "/proc/self/fd/%d", olddirfd);
        len = readlink(procfd, dirpath, sizeof(dirpath) - 1);
        if (len < 0) return -1;
        dirpath[len] = '\0';
        solcompat_snprintf(old_full, sizeof(old_full), "%s/%s", dirpath, oldpath);
    }

    /* Resolve new path */
    if (newpath[0] == '/' || newdirfd == AT_FDCWD) {
        strlcpy(new_full, newpath, sizeof(new_full));
    } else {
        char procfd[64], dirpath[1024];
        ssize_t len;
        solcompat_snprintf(procfd, sizeof(procfd), "/proc/self/fd/%d", newdirfd);
        len = readlink(procfd, dirpath, sizeof(dirpath) - 1);
        if (len < 0) return -1;
        dirpath[len] = '\0';
        solcompat_snprintf(new_full, sizeof(new_full), "%s/%s", dirpath, newpath);
    }

    pthread_mutex_lock(&at_mutex);
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
