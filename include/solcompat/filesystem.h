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

#ifdef __cplusplus
extern "C" {
#endif

/* --- utimes --- */
#ifndef HAVE_UTIMES
struct timeval;  /* forward decl, <sys/time.h> */
int utimes(const char *path, const struct timeval tv[2]);
#endif

/* --- futimens / utimensat --- */
#ifndef HAVE_FUTIMENS
int futimens(int fd, const struct timespec times[2]);
#endif
#ifndef HAVE_UTIMENSAT
#ifndef UTIME_NOW
#define UTIME_NOW  ((1l << 30) - 1l)
#define UTIME_OMIT ((1l << 30) - 2l)
#endif
int utimensat(int dirfd, const char *pathname,
              const struct timespec times[2], int flags);
#endif

/* --- flock --- */
#ifndef LOCK_SH
#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8
#endif
#ifndef HAVE_FLOCK
int flock(int fd, int operation);
#endif

/* --- scandir / alphasort --- */
#ifndef HAVE_SCANDIR
int scandir(const char *dirp, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **));
#endif

#ifndef HAVE_ALPHASORT
int alphasort(const struct dirent **a, const struct dirent **b);
#endif

/* --- fdopendir --- */
#ifndef HAVE_FDOPENDIR
DIR *fdopendir(int fd);
#endif

/* --- dirfd --- */
/*
 * dirfd() returns the file descriptor underlying a DIR stream.
 *
 * Solaris 7's DIR structure has dd_fd as a documented first member, but
 * dirfd() itself is not provided as a libc function.  Our implementation
 * simply returns dirp->dd_fd.
 */
#ifndef HAVE_DIRFD
int dirfd(DIR *dir_stream);
#endif

/* --- posix_fadvise (no-op on Solaris 7) --- */
#ifndef POSIX_FADV_NORMAL
#define POSIX_FADV_NORMAL     0
#define POSIX_FADV_SEQUENTIAL 2
#define POSIX_FADV_RANDOM     1
#define POSIX_FADV_NOREUSE    5
#define POSIX_FADV_WILLNEED   3
#define POSIX_FADV_DONTNEED   4
#endif
#ifndef HAVE_POSIX_FADVISE
int posix_fadvise(int fd, off_t offset, off_t len, int advice);
#endif

/* --- posix_fallocate (emulated via ftruncate) --- */
#ifndef HAVE_POSIX_FALLOCATE
int posix_fallocate(int fd, off_t offset, off_t len);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_FILESYSTEM_H */
