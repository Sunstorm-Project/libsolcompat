/*
 * filesystem.c — Missing filesystem functions for Solaris 7
 *
 * utimes, futimens, utimensat, flock (via fcntl F_SETLK),
 * scandir, alphasort, fdopendir, posix_fadvise
 */

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
     * Best we can do: read the /proc/self/fd/N path and use utime.
     * This is a reasonable approximation.
     */
    char procpath[64];
    char linkpath[1024];
    ssize_t len;

    solcompat_snprintf(procpath, sizeof(procpath), "/proc/self/fd/%d", fd);
    len = readlink(procpath, linkpath, sizeof(linkpath) - 1);
    if (len < 0)
        return -1;
    linkpath[len] = '\0';

    if (times == NULL) {
        return utime(linkpath, NULL);
    } else {
        struct utimbuf ut;
        ut.actime = times[0].tv_sec;
        ut.modtime = times[1].tv_sec;
        return utime(linkpath, &ut);
    }
}

/* UTIME_NOW and UTIME_OMIT defined in filesystem.h */
#ifndef UTIME_NOW
#define UTIME_NOW  ((1l << 30) - 1l)
#define UTIME_OMIT ((1l << 30) - 2l)
#endif

int
utimensat(int dirfd, const char *pathname,
          const struct timespec times[2], int flags)
{
    char fullpath[1024];
    struct stat st;
    struct utimbuf ut;
    time_t now_time;

    (void)flags;  /* AT_SYMLINK_NOFOLLOW not supportable via utime */

    /* Resolve path relative to dirfd */
    if (pathname[0] == '/' || dirfd == -100 /* AT_FDCWD */) {
        strlcpy(fullpath, pathname, sizeof(fullpath));
    } else {
        char dirpath[1024];
        char procfd[64];
        ssize_t len;
        solcompat_snprintf(procfd, sizeof(procfd), "/proc/self/fd/%d", dirfd);
        len = readlink(procfd, dirpath, sizeof(dirpath) - 1);
        if (len < 0) return -1;
        dirpath[len] = '\0';
        solcompat_snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, pathname);
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
     * Solaris 7 has no fdopendir.  Approximate by reading /proc/self/fd/N
     * to find the path and opendir() on it.
     * The fd is NOT consumed by this — caller keeps ownership.
     */
    char procpath[64];
    char dirpath[1024];
    ssize_t len;

    solcompat_snprintf(procpath, sizeof(procpath), "/proc/self/fd/%d", fd);
    len = readlink(procpath, dirpath, sizeof(dirpath) - 1);
    if (len < 0) {
        errno = EBADF;
        return NULL;
    }
    dirpath[len] = '\0';
    return opendir(dirpath);
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
