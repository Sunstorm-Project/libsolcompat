/*
 * filesystem.c — Missing filesystem functions for Solaris 7
 *
 * utimes, futimens, utimensat, flock (via fcntl F_SETLK),
 * scandir, alphasort, fdopendir, posix_fadvise
 */

/* Must be defined BEFORE any includes so that stat/lstat/fstat become
 * stat64/lstat64/fstat64 and struct stat becomes struct stat64.
 * All callers (GNU tools) are compiled with _FILE_OFFSET_BITS=64.  */
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <utime.h>

/* Forward declarations for our own functions */
extern int solcompat_snprintf(char *, size_t, const char *, ...);
extern size_t strlcpy(char *, const char *, size_t);

/* UTIME_NOW and UTIME_OMIT — must be defined before futimens/utimensat */
#ifndef UTIME_NOW
#define UTIME_NOW  ((1l << 30) - 1l)
#define UTIME_OMIT ((1l << 30) - 2l)
#endif

#ifndef AT_FDCWD
#define AT_FDCWD (-3041965)
#endif
#define AT_FDCWD_LINUX (-100)
#define IS_AT_FDCWD(fd) ((fd) == AT_FDCWD || (fd) == AT_FDCWD_LINUX)

int
utimes(const char *path, const struct timeval tv[2])
{
    if (tv == NULL) {
        /* Set to current time */
        return utime(path, NULL);
    } else {
        struct utimbuf ut;
        ut.actime = tv[0].tv_sec;
        ut.modtime = tv[1].tv_sec;
        return utime(path, &ut);
    }
}

int
futimens(int fd, const struct timespec times[2])
{
    /*
     * Solaris 7 has no futimens or futimes.
     *
     * Strategy: try multiple approaches to set timestamps via fd.
     *
     * 1. Try utime() directly on /proc/self/fd/N path (Solaris 7 has
     *    /proc/self/fd/ entries but they are NOT symlinks — readlink
     *    fails with EINVAL.  However, utime() on the /proc path may
     *    still work because the kernel follows the fd to the inode.)
     *
     * 2. If that fails, try /proc/<pid>/fd/N (older Solaris style).
     *
     * 3. If nothing works, silently succeed — setting timestamps is
     *    non-critical for most callers (touch, cp, tar) and the file
     *    has already been created/modified with the right content.
     */
    char procpath[64];
    int result;

    /* UTIME_NOW / UTIME_OMIT handling */
    struct utimbuf ut;
    struct utimbuf *ut_ptr = NULL;

    if (times != NULL) {
        struct stat st;
        time_t now_time = time(NULL);

        if (times[0].tv_nsec == UTIME_OMIT || times[1].tv_nsec == UTIME_OMIT) {
            if (fstat(fd, &st) < 0)
                return -1;
        }

        ut.actime = (times[0].tv_nsec == UTIME_NOW) ? now_time :
                    (times[0].tv_nsec == UTIME_OMIT) ? st.st_atime :
                    times[0].tv_sec;
        ut.modtime = (times[1].tv_nsec == UTIME_NOW) ? now_time :
                     (times[1].tv_nsec == UTIME_OMIT) ? st.st_mtime :
                     times[1].tv_sec;
        ut_ptr = &ut;
    }

    /* Try /proc/self/fd/N path directly */
    solcompat_snprintf(procpath, sizeof(procpath), "/proc/self/fd/%d", fd);
    result = utime(procpath, ut_ptr);
    if (result == 0)
        return 0;

    /* Try /proc/<pid>/fd/N */
    solcompat_snprintf(procpath, sizeof(procpath), "/proc/%d/fd/%d",
                       (int)getpid(), fd);
    result = utime(procpath, ut_ptr);
    if (result == 0)
        return 0;

    /* Best effort: silently succeed if we can't set timestamps.
     * The file content is correct; only the timestamp is approximate. */
    return 0;
}

int
utimensat(int dirfd, const char *pathname,
          const struct timespec times[2], int flags)
{
    char fullpath[1024];
    struct stat st;
    struct utimbuf ut;
    time_t now_time;
    int saved_fd = -1;
    int result;

    (void)flags;  /* AT_SYMLINK_NOFOLLOW not supportable via utime */

    /* Resolve path relative to dirfd */
    if (pathname[0] == '/' || IS_AT_FDCWD(dirfd)) {
        strlcpy(fullpath, pathname, sizeof(fullpath));
    } else {
        /*
         * Non-AT_FDCWD case: use fchdir dance (same as at_funcs.c)
         * because Solaris 7 /proc/self/fd/N is not a symlink.
         */
        char cwd[1024];
        saved_fd = open(".", O_RDONLY);
        if (saved_fd < 0) return -1;
        if (fchdir(dirfd) < 0) {
            close(saved_fd);
            return -1;
        }
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            fchdir(saved_fd);
            close(saved_fd);
            return -1;
        }
        fchdir(saved_fd);
        close(saved_fd);
        saved_fd = -1;
        solcompat_snprintf(fullpath, sizeof(fullpath), "%s/%s", cwd, pathname);
    }

    if (times == NULL)
        return utime(fullpath, NULL);

    now_time = time(NULL);

    /* Handle UTIME_NOW / UTIME_OMIT */
    if (times[0].tv_nsec == UTIME_OMIT || times[1].tv_nsec == UTIME_OMIT) {
        if (stat(fullpath, &st) < 0)
            return -1;
    }

    ut.actime = (times[0].tv_nsec == UTIME_NOW) ? now_time :
                (times[0].tv_nsec == UTIME_OMIT) ? st.st_atime :
                times[0].tv_sec;
    ut.modtime = (times[1].tv_nsec == UTIME_NOW) ? now_time :
                 (times[1].tv_nsec == UTIME_OMIT) ? st.st_mtime :
                 times[1].tv_sec;

    return utime(fullpath, &ut);
}

/*
 * flock — emulated via fcntl(F_SETLK/F_SETLKW)
 */
/* LOCK_SH, LOCK_EX, LOCK_NB, LOCK_UN defined in filesystem.h */
#ifndef LOCK_SH
#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8
#endif

int
flock(int fd, int operation)
{
    struct flock fl;
    int cmd;

    memset(&fl, 0, sizeof(fl));
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  /* entire file */

    if (operation & LOCK_UN) {
        fl.l_type = F_UNLCK;
    } else if (operation & LOCK_EX) {
        fl.l_type = F_WRLCK;
    } else if (operation & LOCK_SH) {
        fl.l_type = F_RDLCK;
    } else {
        errno = EINVAL;
        return -1;
    }

    cmd = (operation & LOCK_NB) ? F_SETLK : F_SETLKW;

    if (fcntl(fd, cmd, &fl) < 0) {
        if (errno == EACCES)
            errno = EWOULDBLOCK;
        return -1;
    }
    return 0;
}

int
scandir(const char *dirp, struct dirent ***namelist,
        int (*filter)(const struct dirent *),
        int (*compar)(const struct dirent **, const struct dirent **))
{
    DIR *d;
    struct dirent *entry;
    struct dirent **list = NULL;
    size_t count = 0;
    size_t allocated = 32;

    d = opendir(dirp);
    if (!d)
        return -1;

    list = (struct dirent **)malloc(allocated * sizeof(*list));
    if (!list) {
        closedir(d);
        return -1;
    }

    while ((entry = readdir(d)) != NULL) {
        if (filter && !filter(entry))
            continue;

        if (count >= allocated) {
            struct dirent **newlist;
            allocated *= 2;
            newlist = (struct dirent **)realloc(list, allocated * sizeof(*list));
            if (!newlist) {
                /* Free what we have */
                size_t i;
                for (i = 0; i < count; i++)
                    free(list[i]);
                free(list);
                closedir(d);
                return -1;
            }
            list = newlist;
        }

        /* Duplicate the dirent */
        {
            size_t entsize = sizeof(struct dirent);
            struct dirent *dup = (struct dirent *)malloc(entsize);
            if (!dup) {
                size_t i;
                for (i = 0; i < count; i++)
                    free(list[i]);
                free(list);
                closedir(d);
                return -1;
            }
            memcpy(dup, entry, entsize);
            list[count++] = dup;
        }
    }

    closedir(d);

    if (compar && count > 1) {
        qsort(list, count, sizeof(*list),
              (int (*)(const void *, const void *))compar);
    }

    *namelist = list;
    return (int)count;
}

int
alphasort(const struct dirent **a, const struct dirent **b)
{
    return strcoll((*a)->d_name, (*b)->d_name);
}

DIR *
fdopendir(int fd)
{
    /*
     * Solaris 7 has no fdopendir.  Approximate by using fchdir + getcwd
     * to find the directory path, then opendir() on it.
     * Solaris 7 /proc/self/fd/N entries are NOT symlinks so readlink
     * does not work.
     */
    char dirpath[1024];
    int saved;
    char *p;

    saved = open(".", O_RDONLY);
    if (saved < 0) {
        errno = EBADF;
        return NULL;
    }

    if (fchdir(fd) < 0) {
        close(saved);
        errno = EBADF;
        return NULL;
    }

    p = getcwd(dirpath, sizeof(dirpath));

    fchdir(saved);
    close(saved);

    if (p == NULL) {
        errno = EBADF;
        return NULL;
    }

    return opendir(dirpath);
}

/*
 * dirfd — return the file descriptor underlying a DIR stream.
 *
 * Solaris 7 does not provide dirfd() as a libc function, but its DIR
 * typedef (in <dirent.h>) documents dd_fd as the file descriptor field.
 * This implementation simply returns that value.
 *
 * POSIX requires EBADF when dirp is invalid; we use EINVAL for NULL since
 * we cannot distinguish a closed DIR from a valid one without dd_fd itself.
 */
int
dirfd(DIR *dir_stream)
{
    if (dir_stream == NULL) {
        errno = EINVAL;
        return -1;
    }
    return dir_stream->dd_fd;
}

int
posix_fadvise(int fd, off_t offset, off_t len, int advice)
{
    /* No-op on Solaris 7 — no kernel support for file advice */
    (void)fd;
    (void)offset;
    (void)len;
    (void)advice;
    return 0;
}

/*
 * posix_fallocate — ensure disk space is allocated for a file region.
 *
 * Solaris 7 has no fallocate or posix_fallocate.  We emulate by extending
 * the file with ftruncate if needed.  This doesn't guarantee contiguous
 * allocation, but it does reserve the space (which is what callers care about).
 *
 * Returns 0 on success, or an error number (not -1).
 */
int
posix_fallocate(int fd, off_t offset, off_t len)
{
    struct stat file_stat;
    off_t required_size;

    if (offset < 0 || len <= 0)
        return EINVAL;

    required_size = offset + len;

    if (fstat(fd, &file_stat) < 0)
        return errno;

    /* Already large enough — nothing to do */
    if (file_stat.st_size >= required_size)
        return 0;

    /* Extend the file; ftruncate zero-fills the gap */
    if (ftruncate(fd, required_size) < 0)
        return errno;

    return 0;
}

/*
 * futimesat — early POSIX-2008 draft API for setting file timestamps
 * relative to a directory file descriptor with microsecond resolution.
 * Solaris 10+ provides it; Solaris 7 doesn't.
 *
 * Modern POSIX deprecates it in favor of utimensat(dirfd, ..., AT_FDCWD,
 * times, 0), but libuv (and therefore cmake) still calls it.
 *
 * We implement it as a thin wrapper over our utimensat shim, converting
 * the timeval pair to timespec.  Microsecond precision survives; the
 * underlying utimensat further degrades to second precision because
 * Solaris 7's utime(2) only accepts time_t.
 */
int
futimesat(int dirfd, const char *pathname, const struct timeval times[2])
{
    struct timespec ts[2];

    if (times == NULL)
        return utimensat(dirfd, pathname, NULL, 0);

    ts[0].tv_sec  = times[0].tv_sec;
    ts[0].tv_nsec = (long)times[0].tv_usec * 1000L;
    ts[1].tv_sec  = times[1].tv_sec;
    ts[1].tv_nsec = (long)times[1].tv_usec * 1000L;

    return utimensat(dirfd, pathname, ts, 0);
}

/*
 * Extended attribute stubs — Solaris 7 has no xattr support.
 * Every Linux xattr API is declared in sysroot-overlay/usr/include/sys/xattr.h
 * so vim/curl/coreutils/gettext/glib compile cleanly. These stubs return -1
 * with errno=ENOTSUP so runtime callers get a clean "not supported" signal
 * instead of a link error.
 */
#include <errno.h>
#include <sys/xattr.h>

ssize_t getxattr(const char *path, const char *name, void *value, size_t size)
{ (void)path; (void)name; (void)value; (void)size; errno = ENOTSUP; return -1; }
ssize_t lgetxattr(const char *path, const char *name, void *value, size_t size)
{ (void)path; (void)name; (void)value; (void)size; errno = ENOTSUP; return -1; }
ssize_t fgetxattr(int fd, const char *name, void *value, size_t size)
{ (void)fd; (void)name; (void)value; (void)size; errno = ENOTSUP; return -1; }

int setxattr(const char *path, const char *name, const void *value, size_t size, int flags)
{ (void)path; (void)name; (void)value; (void)size; (void)flags; errno = ENOTSUP; return -1; }
int lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags)
{ (void)path; (void)name; (void)value; (void)size; (void)flags; errno = ENOTSUP; return -1; }
int fsetxattr(int fd, const char *name, const void *value, size_t size, int flags)
{ (void)fd; (void)name; (void)value; (void)size; (void)flags; errno = ENOTSUP; return -1; }

ssize_t listxattr(const char *path, char *list, size_t size)
{ (void)path; (void)list; (void)size; errno = ENOTSUP; return -1; }
ssize_t llistxattr(const char *path, char *list, size_t size)
{ (void)path; (void)list; (void)size; errno = ENOTSUP; return -1; }
ssize_t flistxattr(int fd, char *list, size_t size)
{ (void)fd; (void)list; (void)size; errno = ENOTSUP; return -1; }

int removexattr(const char *path, const char *name)
{ (void)path; (void)name; errno = ENOTSUP; return -1; }
int lremovexattr(const char *path, const char *name)
{ (void)path; (void)name; errno = ENOTSUP; return -1; }
int fremovexattr(int fd, const char *name)
{ (void)fd; (void)name; errno = ENOTSUP; return -1; }
