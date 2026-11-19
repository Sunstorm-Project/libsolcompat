#include <threads.h>
#include <stdio.h>
static mtx_t lock;
static cnd_t changed;
static tss_t key;
static once_flag once = ONCE_FLAG_INIT;
static int ready, initialized, destructed;
static void initialize(void) { ++initialized; }
static void destructor(void *p) { if (p == &ready) ++destructed; }
static int worker(void *arg)
{
    if (!thrd_equal(thrd_current(),thrd_current()) || tss_set(key,arg) || tss_get(key)!=arg)
        return 9;
    call_once(&once,initialize);
    if (mtx_lock(&lock)) return 8;
    ready=1;
    cnd_signal(&changed);
    mtx_unlock(&lock);
    thrd_exit(-42);
}
static int detached_worker(void *unused)
{
    (void)unused;
    mtx_lock(&lock);
    ready = 2;
    cnd_broadcast(&changed);
    mtx_unlock(&lock);
    return 0;
}

int main(void)
{
    thrd_t t;
    mtx_t recursive;
    struct timespec past={0,0}, delay={0,1000000L};
    int result;
    if (mtx_init(&recursive,mtx_recursive|mtx_timed) || mtx_lock(&recursive) ||
        mtx_trylock(&recursive) || mtx_unlock(&recursive) || mtx_unlock(&recursive)) return 1;
    mtx_destroy(&recursive);
    if (mtx_init(&lock,mtx_timed) || cnd_init(&changed) || tss_create(&key,destructor) ||
        mtx_lock(&lock) || mtx_trylock(&lock)!=thrd_busy ||
        mtx_timedlock(&lock,&past)!=thrd_timedout || cnd_timedwait(&changed,&lock,&past)!=thrd_timedout)
        return 2;
    if (thrd_create(&t,worker,&ready)) return 3;
    while (!ready) if (cnd_wait(&changed,&lock)) return 4;
    mtx_unlock(&lock);
    if (thrd_join(t,&result) || result!=-42 || destructed!=1 || tss_get(key)!=NULL) return 5;
    call_once(&once,initialize);
    thrd_yield();
    if (initialized!=1 || thrd_sleep(&delay,NULL)) return 6;
    if (mtx_lock(&lock) || thrd_create(&t,detached_worker,NULL) || thrd_detach(t)) return 7;
    while (ready != 2) if (cnd_wait(&changed,&lock)) return 8;
    mtx_unlock(&lock);
    cnd_destroy(&changed); mtx_destroy(&lock); tss_delete(key);
    puts("PASS C11 threads, recursive/timed locks, condition waits, once and TLS destructor");
    return 0;
}
