/* C11 threads use the Solaris POSIX threads ABI. Threaded applications
 * link with -pthread so the native thread implementation is selected. */
#include <threads.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <thread.h>

extern int pthread_mutex_timedlock(pthread_mutex_t *, const struct timespec *);

struct start_args {
    thrd_start_t function;
    void *argument;
};

static int thread_result(int error)
{
    switch (error) {
    case 0: return thrd_success;
    case ENOMEM: return thrd_nomem;
    case EBUSY: return thrd_busy;
    case ETIMEDOUT: return thrd_timedout;
    default: return thrd_error;
    }
}

static void *start_thread(void *ptr)
{
    struct start_args args = *(struct start_args *)ptr;
    free(ptr);
    return (void *)(intptr_t)args.function(args.argument);
}

int thrd_create(thrd_t *thread, thrd_start_t function, void *argument)
{
    struct start_args *args = malloc(sizeof(*args));
    if (!args)
        return thrd_nomem;
    args->function = function;
    args->argument = argument;
    int error = pthread_create(thread, NULL, start_thread, args);
    if (error)
        free(args);
    return thread_result(error);
}

int thrd_join(thrd_t thread, int *result)
{
    void *value;
    int error = pthread_join(thread, &value);
    if (!error && result)
        *result = (int)(intptr_t)value;
    return thread_result(error);
}

_Noreturn void thrd_exit(int result)
{
    pthread_exit((void *)(intptr_t)result);
    __builtin_unreachable();
}

int thrd_detach(thrd_t t) { return thread_result(pthread_detach(t)); }
thrd_t thrd_current(void) { return pthread_self(); }
int thrd_equal(thrd_t a, thrd_t b) { return pthread_equal(a, b); }
void thrd_yield(void) { thr_yield(); }

int thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
    if (nanosleep(duration, remaining) == 0)
        return 0;
    return errno == EINTR ? -1 : -2;
}

int mtx_init(mtx_t *mutex, int type)
{
    pthread_mutexattr_t attr;
    if (type & ~(mtx_recursive | mtx_timed))
        return thrd_error;
    if (!(type & mtx_recursive))
        return thread_result(pthread_mutex_init(mutex, NULL));
    int error = pthread_mutexattr_init(&attr);
    if (error)
        return thread_result(error);
    error = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (!error)
        error = pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    return thread_result(error);
}

int mtx_lock(mtx_t *m) { return thread_result(pthread_mutex_lock(m)); }
int mtx_trylock(mtx_t *m) { return thread_result(pthread_mutex_trylock(m)); }
int mtx_timedlock(mtx_t *m, const struct timespec *deadline)
{
    return thread_result(pthread_mutex_timedlock(m, deadline));
}
int mtx_unlock(mtx_t *m) { return thread_result(pthread_mutex_unlock(m)); }
void mtx_destroy(mtx_t *m) { pthread_mutex_destroy(m); }

int cnd_init(cnd_t *c) { return thread_result(pthread_cond_init(c, NULL)); }
int cnd_signal(cnd_t *c) { return thread_result(pthread_cond_signal(c)); }
int cnd_broadcast(cnd_t *c) { return thread_result(pthread_cond_broadcast(c)); }
int cnd_wait(cnd_t *c, mtx_t *m) { return thread_result(pthread_cond_wait(c, m)); }
int cnd_timedwait(cnd_t *c, mtx_t *m, const struct timespec *deadline)
{
    return thread_result(pthread_cond_timedwait(c, m, deadline));
}
void cnd_destroy(cnd_t *c) { pthread_cond_destroy(c); }

int tss_create(tss_t *key, tss_dtor_t dtor)
{
    return thread_result(pthread_key_create(key, dtor));
}
void *tss_get(tss_t key) { return pthread_getspecific(key); }
int tss_set(tss_t key, void *value) { return thread_result(pthread_setspecific(key, value)); }
void tss_delete(tss_t key) { pthread_key_delete(key); }
void call_once(once_flag *flag, void (*function)(void)) { pthread_once(flag, function); }
