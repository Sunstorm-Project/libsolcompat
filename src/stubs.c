/*
 * stubs.c — Stubbed locale and threading helpers for Solaris 7
 *
 * pthread_setname_np (no-op), newlocale/uselocale/freelocale
 *
 * Solaris 7 has no per-thread locale support.  These stubs allow
 * software that uses the POSIX 2008 locale API to compile and run,
 * but all locales are effectively the global locale.
 */

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <pthread.h>

#ifndef HAVE_PTHREAD_SETNAME_NP
int
pthread_setname_np(pthread_t thread, const char *name)
{
    /* Solaris 7 has no thread naming API — silently succeed */
    (void)thread;
    (void)name;
    return 0;
}
#endif

/*
 * Per-thread locale stubs.
 * These are not real per-thread locales — they just call setlocale()
 * which affects the whole process.  This is sufficient for software
 * that uses newlocale/uselocale for "set to C locale temporarily"
 * patterns, which is the most common use case.
 */

/* Our locale_t is just a pointer to a malloc'd locale name string */
typedef void *solcompat_locale_t;

#ifndef HAVE_USELOCALE

/* Table of category names for setlocale */
static int
mask_to_category(int mask)
{
    /* Return the first matching category */
    if (mask & (1 << 0 /* LC_CTYPE */))  return LC_CTYPE;
    if (mask & (1 << 1 /* LC_NUMERIC */))return LC_NUMERIC;
    if (mask & (1 << 2 /* LC_TIME */))   return LC_TIME;
    if (mask & (1 << 3 /* LC_COLLATE */))return LC_COLLATE;
    if (mask & (1 << 4 /* LC_MONETARY */))return LC_MONETARY;
    if (mask & (1 << 5 /* LC_MESSAGES */))return LC_MESSAGES;
    return LC_ALL;
}

void *
newlocale(int category_mask, const char *locale, void *base)
{
    char *loc;
    (void)category_mask;

    if (base) {
        free(base);
    }

    loc = strdup(locale ? locale : "C");
    if (!loc) {
        errno = ENOMEM;
        return NULL;
    }

    return (void *)loc;
}

void *
uselocale(void *newloc)
{
    static void *current = NULL;
    void *old = current;

    if (newloc == (void *)-1 /* LC_GLOBAL_LOCALE */) {
        /* Query only */
        return old ? old : (void *)-1;
    }

    if (newloc) {
        const char *name = (const char *)newloc;
        setlocale(LC_ALL, name);
        current = newloc;
    }

    return old ? old : (void *)-1;
}

void
freelocale(void *locobj)
{
    if (locobj && locobj != (void *)-1)
        free(locobj);
}

void *
duplocale(void *locobj)
{
    if (!locobj || locobj == (void *)-1)
        return newlocale(0, "C", NULL);

    return newlocale(0, (const char *)locobj, NULL);
}

#endif /* HAVE_USELOCALE */

/*
 * sem_timedwait — wait on a semaphore with a timeout.
 *
 * Solaris 7 has sem_wait() and sem_trywait() (POSIX.1b real-time) but
 * not sem_timedwait() (POSIX.1-2001).  We emulate by polling sem_trywait()
 * with nanosleep() intervals.
 *
 * This is not efficient for long waits, but Python's usage is primarily
 * for thread synchronization with short timeouts.  For the single-CPU
 * QEMU target, the polling approach is acceptable.
 */
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>

#ifndef HAVE_SEM_TIMEDWAIT
int
sem_timedwait(sem_t *sem, const struct timespec *abs_timeout)
{
    struct timespec poll_interval;
    struct timeval now;

    /* 10ms poll interval — balances responsiveness vs CPU */
    poll_interval.tv_sec = 0;
    poll_interval.tv_nsec = 10000000L; /* 10 ms */

    for (;;) {
        /* Try to acquire immediately */
        if (sem_trywait(sem) == 0)
            return 0;

        if (errno != EAGAIN)
            return -1; /* Real error, not just "would block" */

        /* Check if we've exceeded the deadline */
        gettimeofday(&now, NULL);
        if (now.tv_sec > abs_timeout->tv_sec ||
            (now.tv_sec == abs_timeout->tv_sec &&
             now.tv_usec * 1000L >= abs_timeout->tv_nsec)) {
            errno = ETIMEDOUT;
            return -1;
        }

        nanosleep(&poll_interval, NULL);
    }
}
#endif /* HAVE_SEM_TIMEDWAIT */
