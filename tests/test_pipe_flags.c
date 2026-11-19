#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "solcompat/process.h"

int main(void)
{
    int ends[2] = {-11, -12};
    int i, fd, status;
    char byte;
    if (pipe2(ends, -1) != -1 || errno != EINVAL ||
        ends[0] != -11 || ends[1] != -12)
        return 1;
    if (pipe2(ends, O_NONBLOCK | O_CLOEXEC))
        return 2;
    for (i = 0; i < 2; i++) {
        status = fcntl(ends[i], F_GETFL);
        if (status < 0 || !(status & O_NONBLOCK) ||
            fcntl(ends[i], F_GETFD) != FD_CLOEXEC)
            return 3;
    }
    fd = dup(ends[0]);
    if (fd < 0 || dup3(ends[1], fd, -1) != -1 || errno != EINVAL)
        return 4;
    if (write(ends[1], "x", 1) != 1 || read(fd, &byte, 1) != 1 || byte != 'x')
        return 5;
    if (dup3(ends[1], fd, O_CLOEXEC) != fd || fcntl(fd, F_GETFD) != FD_CLOEXEC)
        return 6;
    close(fd);
    close(ends[0]);
    close(ends[1]);
    return 0;
}
