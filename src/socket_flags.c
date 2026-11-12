/* Solaris 7 accepts native socket types, without modern creation flags.
 * Keep the native ABI intact and redirect rebuilt callers through these
 * prefixed entry points. Flag application is not atomic with creation. */
#define SOLCOMPAT_NATIVE_SOCKET 1
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0x80000
#endif
#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK 0x800
#endif

static int apply_socket_flags(int fd, int flags)
{
    int value;
    if (flags & SOCK_CLOEXEC) {
        value = fcntl(fd, F_GETFD);
        if (value < 0 || fcntl(fd, F_SETFD, value | FD_CLOEXEC) < 0)
            return -1;
    }
    if (flags & SOCK_NONBLOCK) {
        value = fcntl(fd, F_GETFL);
        if (value < 0 || fcntl(fd, F_SETFL, value | O_NONBLOCK) < 0)
            return -1;
    }
    return 0;
}

int solcompat_socket(int domain, int type, int protocol)
{
    const int flags = type & (SOCK_CLOEXEC | SOCK_NONBLOCK);
    int fd = socket(domain, type & ~(SOCK_CLOEXEC | SOCK_NONBLOCK), protocol);
    if (fd >= 0 && apply_socket_flags(fd, flags) < 0) {
        int saved = errno;
        close(fd);
        errno = saved;
        return -1;
    }
    return fd;
}

int solcompat_socketpair(int domain, int type, int protocol, int pair[2])
{
    const int flags = type & (SOCK_CLOEXEC | SOCK_NONBLOCK);
    int created[2], result;
    if (!pair) { errno = EFAULT; return -1; }
    result = socketpair(domain, type & ~(SOCK_CLOEXEC | SOCK_NONBLOCK), protocol, created);
    if (result == 0 && (apply_socket_flags(created[0], flags) < 0 ||
                        apply_socket_flags(created[1], flags) < 0)) {
        int saved = errno;
        close(created[0]);
        close(created[1]);
        errno = saved;
        return -1;
    }
    if (result == 0) { pair[0] = created[0]; pair[1] = created[1]; }
    return result;
}
