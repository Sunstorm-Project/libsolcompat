#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <solcompat/poll_ext.h>

/* Replace the underlying wait to exercise multi-week timeouts immediately. */
static int waits[2], calls;
int poll(struct pollfd *fds, nfds_t n, int timeout)
{
    (void)fds;
    (void)n;
    if (calls >= 2)
        _exit(8);
    waits[calls++] = timeout;
    return 0;
}

int main(void)
{
    struct timespec ts = { 0, 1 };
    if (ppoll(NULL, 0, &ts, NULL) || calls != 1 || waits[0] != 1)
        return 1;
    ts.tv_nsec = 1000000000L;
    if (ppoll(NULL, 0, &ts, NULL) != -1 || errno != EINVAL || calls != 1)
        return 2;
    calls = 0;
    ts.tv_sec = INT_MAX / 1000 + 1;
    ts.tv_nsec = 0;
    if (ppoll(NULL, 0, &ts, NULL) || calls != 2 ||
        waits[0] != INT_MAX || waits[1] != 1000 - INT_MAX % 1000)
        return 3;
    return 0;
}
