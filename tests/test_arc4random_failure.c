/* Run in a Solaris test root without random devices or an EGD socket. */
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/random.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <solcompat/random.h>

/* The server returns a known pattern in partial replies. A 300-byte request
 * crosses the single EGD command size limit. */
static int test_egd(void)
{
    struct sockaddr_un addr;
    unsigned char bytes[300];
    static char setting[128];
    int server, status;
    pid_t child;
    size_t i;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    sprintf(addr.sun_path, "/tmp/test-egd-%ld", (long)getpid());
    sprintf(setting, "SOLCOMPAT_EGD_SOCKET=%s", addr.sun_path);
    server = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server < 0 || bind(server, (struct sockaddr *)&addr, sizeof(addr)) ||
        listen(server, 1) || putenv(setting))
        return 5;
    child = fork();
    if (child == 0) {
        int fd = accept(server, NULL, NULL);
        size_t sent = 0;
        alarm(10);
        while (sent < sizeof(bytes)) {
            unsigned char cmd, count;
            if (read(fd, &cmd, 1) != 1 || read(fd, &count, 1) != 1 ||
                cmd != 2 || count == 0)
                _exit(1);
            while (count--) {
                unsigned char value = (unsigned char)sent++;
                if (write(fd, &value, 1) != 1)
                    _exit(2);
            }
        }
        close(fd);
        fd = accept(server, NULL, NULL);
        unsigned char cmd, count;
        unsigned char reply[] = { 2, 0x23, 0x45 };
        if (fd < 0 || read(fd, &cmd, 1) != 1 || read(fd, &count, 1) != 1 ||
            cmd != 1 || count != 4 || write(fd, reply, sizeof(reply)) != sizeof(reply))
            _exit(3);
        _exit(sent != sizeof(bytes));
    }
    if (child < 0)
        return 6;
    alarm(15);
    arc4random_buf(bytes, sizeof(bytes));
    unsigned char partial[4] = { 0 };
    if (getrandom(partial, sizeof(partial), GRND_NONBLOCK) != 2 ||
        partial[0] != 0x23 || partial[1] != 0x45 || partial[2] != 0)
        return 11;
    close(server);
    unlink(addr.sun_path);
    if (waitpid(child, &status, 0) != child || status != 0)
        return 7;
    alarm(0);
    for (i = 0; i < sizeof(bytes); i++)
        if (bytes[i] != (unsigned char)i)
            return 8;
    return 0;
}

int main(void)
{
    unsigned char byte;
    int status;
    pid_t child;

    arc4random_buf(&byte, 0);
    if (arc4random_uniform(1) != 0)
        return 1;
    if (putenv("SOLCOMPAT_ALLOW_WEAK_RANDOM=1") ||
        putenv("SOLCOMPAT_EGD_SOCKET=/missing-egd-for-test"))
        return 2;
    if (getentropy(&byte, 1) != -1 || errno != EIO ||
        getrandom(&byte, 1, 0) != -1 || errno != EIO ||
        getrandom(&byte, 1, GRND_NONBLOCK) != -1 || errno != EAGAIN ||
        getrandom(&byte, 0, 0) != 0 ||
        getrandom(&byte, 0, ~0U) != -1 || errno != EINVAL ||
        getrandom(&byte, 1, GRND_RANDOM | GRND_INSECURE) != -1 || errno != EINVAL)
        return 10;
    child = fork();
    if (child < 0)
        return 3;
    if (child == 0) {
        arc4random_buf(&byte, 1);
        _exit(1);
    }
    if (waitpid(child, &status, 0) != child)
        return 4;
    if (!WIFSIGNALED(status) || WTERMSIG(status) != SIGABRT)
        return 9;
    return test_egd();
}
