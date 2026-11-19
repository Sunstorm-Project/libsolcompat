#include <fcntl.h>
#include <sys/stat.h>
#include <solcompat/filesystem.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    char path[] = "/tmp/space-XXXXXX";
    int fd = mkstemp(path), p[2];
    struct stat st;
    if (fd < 0 || unlink(path)) return 1;
    errno = EINTR;
    if (posix_fallocate(fd, 0, 1048576) != EINVAL || errno != EINTR ||
        fstat(fd,&st) || st.st_size || posix_fallocate(-1,0,1) != EBADF ||
        posix_fadvise(fd,0,0,POSIX_FADV_RANDOM) ||
        posix_fadvise(fd,0,0,-1) != EINVAL || posix_fadvise(-1,0,1,0) != EBADF ||
        pipe(p) || posix_fadvise(p[1],0,1,0) != ESPIPE ||
        posix_fallocate(p[1],0,1) != ESPIPE)
        return 2;
    off_t max = sizeof(off_t) == 4 ? INT_MAX : (off_t)0x7fffffffffffffffLL;
    if (posix_fallocate(fd,max,1) != EFBIG) return 3;
    close(p[0]); close(p[1]); close(fd);
    puts("PASS space reservation, descriptor errors, advice and offset overflow");
    return 0;
}
