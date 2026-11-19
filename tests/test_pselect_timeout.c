#include <solcompat/poll_ext.h>
#include <errno.h>

static struct timeval actual;
static int calls;
int select(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    if (!t)
        return -1;
    actual = *t;
    ++calls;
    return 0;
}

int main(void)
{
    struct timespec ts = {0, 1};
    if (pselect(0, 0, 0, 0, &ts, 0) || actual.tv_sec || actual.tv_usec != 1)
        return 1;
    ts.tv_nsec = 999999999L;
    if (pselect(0, 0, 0, 0, &ts, 0) || actual.tv_sec != 1 || actual.tv_usec)
        return 2;
    ts.tv_nsec = -1;
    if (pselect(0, 0, 0, 0, &ts, 0) != -1 || errno != EINVAL || calls != 2)
        return 3;
    return 0;
}
