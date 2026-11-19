#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
extern int pthread_mutex_timedlock(pthread_mutex_t *, const struct timespec *);
static pthread_mutex_t shared = PTHREAD_MUTEX_INITIALIZER;
static int acquired;

static void *waiter(void *unused)
{
    struct timespec deadline;
    (void)unused;
    if (clock_gettime(CLOCK_REALTIME, &deadline))
        return NULL;
    deadline.tv_sec += 5;
    if (pthread_mutex_timedlock(&shared, &deadline) == 0) {
        acquired = 1;
        pthread_mutex_unlock(&shared);
    }
    return NULL;
}

int main(void)
{
    pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    struct timespec bad = {0, 1000000000L}, past = {0, 0}, soon;
    if (pthread_mutex_timedlock(&mutex, &past) ||
        pthread_mutex_timedlock(&mutex, &bad) != EINVAL ||
        pthread_mutex_timedlock(&mutex, &past) != ETIMEDOUT ||
        pthread_mutex_unlock(&mutex))
        return 1;
    if (pthread_rwlock_timedrdlock(&rw, &past) ||
        pthread_rwlock_timedwrlock(&rw, &bad) != EINVAL ||
        clock_gettime(CLOCK_REALTIME, &soon))
        return 2;
    soon.tv_nsec += 2000000L;
    if (soon.tv_nsec >= 1000000000L) {
        soon.tv_sec++;
        soon.tv_nsec -= 1000000000L;
    }
    if (pthread_rwlock_timedwrlock(&rw, &soon) != ETIMEDOUT ||
        pthread_rwlock_unlock(&rw) || pthread_rwlock_timedwrlock(&rw, &bad) ||
        pthread_rwlock_unlock(&rw))
        return 3;
    pthread_t thread;
    struct timespec delay = {0, 30000000L};
    if (pthread_mutex_lock(&shared) || pthread_create(&thread, NULL, waiter, NULL))
        return 4;
    nanosleep(&delay, NULL);
    if (pthread_mutex_unlock(&shared) || pthread_join(thread, NULL) || !acquired)
        return 5;
    puts("PASS timed locks, contention, immediate acquisition and invalid deadlines");
    return 0;
}
