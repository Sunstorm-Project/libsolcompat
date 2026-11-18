#include <pthread.h>
#include <errno.h>

int main(void)
{
    pthread_mutexattr_t attr;
    int mode;
    if (pthread_mutexattr_init(&attr) != 0)
        return 1;
    if (pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST) != ENOTSUP ||
        pthread_mutexattr_setrobust(&attr, -1) != EINVAL ||
        pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_STALLED) != 0 ||
        pthread_mutexattr_getrobust(&attr, &mode) != 0 ||
        mode != PTHREAD_MUTEX_STALLED)
        return 2;
    return pthread_mutexattr_destroy(&attr) != 0;
}
