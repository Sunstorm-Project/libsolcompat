#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
extern int futimens(int, const struct timespec *);
extern int utimensat(int, const char *, const struct timespec *, int);
extern int futimesat(int, const char *, const struct timeval *);
int main(void) {
    struct timespec ts[2];
    struct stat before, after;
    int fd = open("timestamp-target", O_CREAT|O_RDWR|O_TRUNC, 0600), r;
    if (fd < 0) return 1;
    ts[0].tv_sec = ts[1].tv_sec = 1000000000;
    ts[0].tv_nsec = ts[1].tv_nsec = 0;
    if (utimensat(-100,"timestamp-target",ts,0)) return 2;
    if (stat("timestamp-target",&before)) return 3;
    unlink("timestamp-link");
    if (symlink("timestamp-target","timestamp-link")) return 4;
    ts[0].tv_sec = ts[1].tv_sec = 1100000000;
    errno=0;
    r=utimensat(-100,"timestamp-link",ts,0x100);
    if (r != -1 || errno != ENOSYS) return 5;
    if (stat("timestamp-target",&after) || after.st_mtime != before.st_mtime) return 6;
    errno=0;
    if (utimensat(-100,"timestamp-target",ts,0x4000) != -1 || errno != EINVAL) return 7;
    errno=0;
    if (futimens(-1,ts) != -1 || errno != EBADF) return 8;
    ts[0].tv_nsec = 1000000000L;
    errno=0;
    if (futimens(fd,ts) != -1 || errno != EINVAL) return 9;
    errno=0;
    if (utimensat(-100,"timestamp-target",ts,0) != -1 || errno != EINVAL) return 10;
    ts[0].tv_nsec = ts[1].tv_nsec = 0;
    errno=0; r=futimens(fd,ts);
    if (r != 0 && !(r == -1 && errno == ENOSYS)) return 11;
    {
        struct timeval tv[2];
        tv[0].tv_sec=tv[1].tv_sec=1200000000;
        tv[0].tv_usec=tv[1].tv_usec=0;
        errno=0; r=futimesat(fd,NULL,tv);
        if (r != 0 && !(r == -1 && errno == ENOSYS)) return 12;
        errno=0; r=futimesat(fd,NULL,NULL);
        if (r != 0 && !(r == -1 && errno == ENOSYS)) return 13;
        errno=0;
        if (futimesat(-1,NULL,tv) != -1 || errno != EBADF) return 14;
        tv[0].tv_usec=1000000;
        errno=0;
        if (futimesat(fd,NULL,tv) != -1 || errno != EINVAL) return 15;
        tv[0].tv_usec=-1;
        errno=0;
        if (futimesat(-100,"timestamp-target",tv) != -1 || errno != EINVAL) return 16;
        errno=0;
        if (utimensat(-100,NULL,ts,0) != -1 || errno != EFAULT) return 17;
    }
    close(fd); unlink("timestamp-link"); unlink("timestamp-target");
    puts("timestamp arguments, no-follow preservation, and fallback protocol passed");
    return 0;
}
