/*
 * solcompat/filesystem.h — Missing filesystem functions
 *
 * utimes, futimens, utimensat, flock, scandir, alphasort,
 * fdopendir, posix_fadvise
 */
#ifndef SOLCOMPAT_FILESYSTEM_H
#define SOLCOMPAT_FILESYSTEM_H

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- utimes --- */
int utimes(const char *path, const struct timeval *tv);

/* --- futimens / utimensat --- */
int futimens(int fd, const struct timespec *times);

#ifndef UTIME_NOW
#define UTIME_NOW  ((1l << 30) - 1l)
#define UTIME_OMIT ((1l << 30) - 2l)
#endif
int utimensat(int dirfd, const char *pathname,
              const struct timespec *times, int flags);

/* --- flock --- */
#ifndef LOCK_SH
#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8
#endif
int flock(int fd, int operation);

/* --- scandir / alphasort --- */
int scandir(const char *dirp, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **));

int alphasort(const struct dirent **a, const struct dirent **b);

/* --- fdopendir --- */
DIR *fdopendir(int fd);

/* --- dirfd --- */
/*
 * dirfd() returns the file descriptor underlying a DIR stream.
 *
 * Solaris 7's DIR structure has dd_fd as a documented first member, but
 * dirfd() itself is not provided as a libc function.  Our implementation
 * simply returns dirp->dd_fd.
 */
int dirfd(DIR *dir_stream);

/* --- posix_fadvise (no-op on Solaris 7) --- */
#ifndef POSIX_FADV_NORMAL
#define POSIX_FADV_NORMAL     0
#define POSIX_FADV_SEQUENTIAL 2
#define POSIX_FADV_RANDOM     1
#define POSIX_FADV_NOREUSE    5
#define POSIX_FADV_WILLNEED   3
#define POSIX_FADV_DONTNEED   4
#endif
int posix_fadvise(int fd, off_t offset, off_t len, int advice);

/* --- posix_fallocate (emulated via ftruncate) --- */
int posix_fallocate(int fd, off_t offset, off_t len);

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_FILESYSTEM_H */
