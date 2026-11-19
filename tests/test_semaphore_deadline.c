#include <errno.h>
#include <signal.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <solcompat/process.h>

static void on_alarm(int signo) { (void)signo; }

int main(void)
{
    sem_t sem;
    struct sigaction sa = {0};
    struct timespec ts = {0, 1000000000L};
    if (sem_init(&sem, 0, 1) || sem_timedwait(&sem, &ts))
        return 1;
    if (sem_timedwait(&sem, &ts) != -1 || errno != EINVAL)
        return 2;
    ts.tv_nsec = 0;
    if (sem_timedwait(&sem, &ts) != -1 || errno != ETIMEDOUT)
        return 3;
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL))
        return 4;
    ts.tv_sec = time(NULL) + 4;
    alarm(1);
    if (sem_timedwait(&sem, &ts) != -1 || errno != EINTR)
        return 5;
    return sem_destroy(&sem) != 0;
}
