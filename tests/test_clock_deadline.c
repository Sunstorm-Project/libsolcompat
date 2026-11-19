#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <solcompat/clock.h>

static void alarm_handler(int signo) { (void)signo; }

int main(void)
{
    struct timespec a, b, res, req = {0, 0}, remain = {123, 456};
    struct sigaction sa = {0};
    if (solcompat_clock_gettime(0, &a) || solcompat_clock_gettime(3, &b) ||
        solcompat_clock_getres(3, &res) || b.tv_sec < a.tv_sec ||
        b.tv_sec - a.tv_sec > 1 || res.tv_nsec != 1000)
        return 1;
    if (clock_nanosleep(3, TIMER_ABSTIME, &req, &remain) ||
        remain.tv_sec != 123 || remain.tv_nsec != 456)
        return 2;
    if (clock_nanosleep(-5, 0, &req, NULL) != EINVAL ||
        clock_nanosleep(4, 2, &req, NULL) != EINVAL)
        return 3;
    req.tv_nsec = 1000000000L;
    if (clock_nanosleep(4, 0, &req, NULL) != EINVAL)
        return 4;
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL))
        return 5;
    req.tv_sec = 4;
    req.tv_nsec = 0;
    alarm(1);
    if (clock_nanosleep(4, 0, &req, &remain) != EINTR ||
        remain.tv_sec < 0 || remain.tv_sec >= 4 ||
        remain.tv_nsec < 0 || remain.tv_nsec >= 1000000000L)
        return 6;
    return 0;
}
