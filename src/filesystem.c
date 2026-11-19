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

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <sys/syscall.h>
#include "dirfd_internal.h"

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
#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 0x100
#endif
#define AT_FDCWD_LINUX (-100)
#define IS_AT_FDCWD(fd) ((fd) == AT_FDCWD || (fd) == AT_FDCWD_LINUX)

int utimes(const char *path, const struct timeval tv[2])
{
    /* Keep the historical libsolcompat symbol while using the Solaris 7
     * syscall, which accepts microseconds and validates the timeval. */
    return syscall(SYS_utimes, path, tv);
}

static int valid_times(const struct timespec times[2])
{
    if (times)
        for (int i = 0; i < 2; ++i) {
            long ns = times[i].tv_nsec;
            if (ns != UTIME_NOW && ns != UTIME_OMIT && (ns < 0 || ns >= 1000000000L)) {
                errno = EINVAL;
                return 0;
            }
        }
    return 1;
}

static int set_times(const char *path, const struct timespec times[2], const struct stat *original)
{
    struct timeval tv[2], now;
    struct stat st;
    if (!times || (times[0].tv_nsec == UTIME_NOW && times[1].tv_nsec == UTIME_NOW))
        return utimes(path, NULL);
    if (times[0].tv_nsec == UTIME_OMIT && times[1].tv_nsec == UTIME_OMIT)
        return 0;
    if (!original && (times[0].tv_nsec == UTIME_OMIT || times[1].tv_nsec == UTIME_OMIT)) {
        if (stat(path, &st) < 0)
            return -1;
        original = &st;
    }
    if ((times[0].tv_nsec == UTIME_NOW || times[1].tv_nsec == UTIME_NOW) &&
        gettimeofday(&now, NULL) < 0)
        return -1;
    for (int i = 0; i < 2; ++i) {
        if (times[i].tv_nsec == UTIME_NOW) {
            tv[i] = now;
        } else if (times[i].tv_nsec == UTIME_OMIT) {
            tv[i].tv_sec = i ? original->st_mtim.tv_sec : original->st_atim.tv_sec;
            tv[i].tv_usec = (i ? original->st_mtim.tv_nsec : original->st_atim.tv_nsec) / 1000;
        } else {
            tv[i].tv_sec = times[i].tv_sec;
            tv[i].tv_usec = times[i].tv_nsec / 1000;
        }
    }
    return utimes(path, tv);
}

int futimens(int fd, const struct timespec times[2])
{
    struct stat st;
    char path[64];
    if (fstat(fd, &st) < 0 || !valid_times(times))
        return -1;
    solcompat_snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
    if (set_times(path, times, &st) == 0)
        return 0;
    solcompat_snprintf(path, sizeof(path), "/proc/%d/fd/%d", (int)getpid(), fd);
    if (set_times(path, times, &st) == 0)
        return 0;
    /* gnulib uses this result to retry using a known pathname. */
    errno = ENOSYS;
    return -1;
}

static int path_times(const char *path, void *argument)
{
    return set_times(path, argument, NULL);
}

int utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags)
{
    if (flags & ~AT_SYMLINK_NOFOLLOW) {
        errno = EINVAL;
        return -1;
    }
    if (flags & AT_SYMLINK_NOFOLLOW) {
        errno = ENOSYS;
        return -1;
    }
    if (!pathname) {
        errno = EFAULT;
        return -1;
    }
    if (!valid_times(times))
        return -1;
    return solcompat_with_dirfd(dirfd, pathname, path_times, (void *)times);
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

DIR *
fdopendir(int fd)
{
    struct stat st;
    DIR *stream;
    int placeholder, flags;

    if (fstat(fd, &st) < 0)
        return NULL;
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return NULL;
    }
    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
        return NULL;
    if ((flags & O_ACCMODE) == O_WRONLY) {
        errno = EBADF;
        return NULL;
    }
    /* Let libc initialize its own stream buffer and private locking state.
     * Its documented dd_fd is the descriptor read and closed by libc.
     * No directory entries have been read from the placeholder. */
    stream = opendir("/");
    if (!stream)
        return NULL;
    placeholder = stream->dd_fd;
    stream->dd_fd = fd;
    close(placeholder);
    return stream;
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
    struct stat st;
    int saved = errno;
    if (offset < 0 || len < 0 || advice < POSIX_FADV_NORMAL || advice > POSIX_FADV_NOREUSE)
        return EINVAL;
    if (fstat(fd, &st) < 0) {
        int error = errno;
        errno = saved;
        return error;
    }
    /* Solaris can disregard a valid access-pattern hint. */
    return S_ISFIFO(st.st_mode) ? ESPIPE : 0;
}

static int allocation_status(int fd, off_t offset, off_t len, off_t maximum)
{
    struct stat st;
    int flags;
    if (offset < 0 || len <= 0)
        return EINVAL;
    if (offset > maximum - len)
        return EFBIG;
    flags = fcntl(fd, F_GETFL);
    if (flags < 0 || fstat(fd, &st) < 0)
        return errno;
    if ((flags & O_ACCMODE) == O_RDONLY)
        return EBADF;
    if (S_ISFIFO(st.st_mode))
        return ESPIPE;
    if (!S_ISREG(st.st_mode))
        return ENODEV;
    /* Solaris 7 reserves F_ALLOCSP, but UFS rejects it. ftruncate creates
     * holes and cannot guarantee future writes against ENOSPC. GCC's module
     * writer understands EINVAL as the unsupported-filesystem result. */
    return EINVAL;
}

int posix_fallocate(int fd, off_t offset, off_t len)
{
    int saved = errno;
    int result = allocation_status(fd, offset, len, INT64_MAX);
    errno = saved;
    return result;
}

int solcompat_posix_fallocate32(int fd, int32_t offset, int32_t len)
{
    int saved = errno;
    int result = allocation_status(fd, offset, len, INT32_MAX);
    errno = saved;
    return result;
}

int solcompat_posix_fadvise32(int fd, int32_t offset, int32_t len, int advice)
{
    return posix_fadvise(fd, offset, len, advice);
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

    /* GNU/Solaris futimesat permits NULL to select dirfd itself.  Keep
     * utimensat's NULL-path EFAULT contract separate; descriptor failure
     * must preserve futimens' ENOSYS pathname-fallback protocol. */
    if (times == NULL)
        return pathname == NULL ? futimens(dirfd, NULL)
                                : utimensat(dirfd, pathname, NULL, 0);

    if (times[0].tv_usec < 0 || times[0].tv_usec > 999999
        || times[1].tv_usec < 0 || times[1].tv_usec > 999999) {
        errno = EINVAL;
        return -1;
    }

    ts[0].tv_sec  = times[0].tv_sec;
    ts[0].tv_nsec = (long)times[0].tv_usec * 1000L;
    ts[1].tv_sec  = times[1].tv_sec;
    ts[1].tv_nsec = (long)times[1].tv_usec * 1000L;

    return pathname == NULL ? futimens(dirfd, ts)
                            : utimensat(dirfd, pathname, ts, 0);
}

/*
 * Extended attribute stubs — Solaris 7 has no xattr support.
 * Every Linux xattr API is declared in sysroot-overlay/usr/include/sys/xattr.h
 * so vim/curl/coreutils/gettext/glib compile cleanly. These stubs return -1
 * with errno=ENOTSUP so runtime callers get a clean "not supported" signal
 * instead of a link error.
 */
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
/* Don't #include <sys/xattr.h> — libsolcompat compiles itself before its
   own sysroot-overlay headers are staged. Re-declare the prototypes here
   matching sysroot-overlay/usr/include/sys/xattr.h exactly. Any signature
   drift here will show up as link errors in xattr-using packages. */
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
