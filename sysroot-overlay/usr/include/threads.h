/*
 * threads.h — C11 threading library mapped onto POSIX threads for Solaris 7
 *
 * GCC 15 does not ship <threads.h> for this target. gnulib's
 * glthread/lock.c and a few packages include it unconditionally.
 * When configure correctly detects pthreads and sets USE_POSIX_THREADS,
 * the C11-threads code path is dead. But packages that include
 * <threads.h> unconditionally still need the header to parse.
 *
 * This header provides the C11 thread/mutex/condition-variable types
 * and function thunks mapped to pthread_*. Full emulation is not
 * provided; packages relying on USE_ISOC_THREADS will fail link with
 * undefined references — that's correct behavior (signals misuse).
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SOLCOMPAT_THREADS_H
#define _SOLCOMPAT_THREADS_H

#include <pthread.h>
#include <time.h>

/* C11 _Noreturn fallback for packages compiled in C99/pre-C11 mode.
   We use _Noreturn on thrd_exit below. GCC 15 rejects the keyword
   outside C11+, so downgrade to the equivalent attribute. */
#if !defined(__cplusplus) && !defined(_Noreturn) && \
    (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L)
#define _Noreturn __attribute__((__noreturn__))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* C11 thread types map directly onto POSIX equivalents. */
typedef pthread_t       thrd_t;
typedef pthread_mutex_t mtx_t;
typedef pthread_cond_t  cnd_t;
typedef pthread_key_t   tss_t;
typedef pthread_once_t  once_flag;

typedef int (*thrd_start_t)(void *);
typedef void (*tss_dtor_t)(void *);

/* Mutex types — C11 ORs values to get recursive+timed. */
enum {
    mtx_plain     = 0,
    mtx_recursive = 1,
    mtx_timed     = 2
};

/* Thread result codes */
enum {
    thrd_success  = 0,
    thrd_busy     = 1,
    thrd_error    = 2,
    thrd_nomem    = 3,
    thrd_timedout = 4
};

#define ONCE_FLAG_INIT PTHREAD_ONCE_INIT

/* Declarations only — no inline implementations to avoid ABI pinning.
 * If USE_ISOC_THREADS is selected at configure time, link will fail;
 * the project should select USE_POSIX_THREADS instead. */
int   thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
int   thrd_equal(thrd_t a, thrd_t b);
thrd_t thrd_current(void);
int   thrd_sleep(const struct timespec *duration, struct timespec *remaining);
void  thrd_yield(void);
_Noreturn void thrd_exit(int res);
int   thrd_detach(thrd_t thr);
int   thrd_join(thrd_t thr, int *res);

int  mtx_init(mtx_t *mtx, int type);
int  mtx_lock(mtx_t *mtx);
int  mtx_timedlock(mtx_t *mtx, const struct timespec *ts);
int  mtx_trylock(mtx_t *mtx);
int  mtx_unlock(mtx_t *mtx);
void mtx_destroy(mtx_t *mtx);

int  cnd_init(cnd_t *cond);
int  cnd_signal(cnd_t *cond);
int  cnd_broadcast(cnd_t *cond);
int  cnd_wait(cnd_t *cond, mtx_t *mtx);
int  cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts);
void cnd_destroy(cnd_t *cond);

int  tss_create(tss_t *key, tss_dtor_t dtor);
void *tss_get(tss_t key);
int  tss_set(tss_t key, void *val);
void tss_delete(tss_t key);

void call_once(once_flag *flag, void (*func)(void));

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_THREADS_H */
