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

int
pthread_getname_np(pthread_t thread, char *name, size_t len)
{
    /* Solaris 7 has no thread naming API — return empty string */
    (void)thread;
    if (len > 0)
        name[0] = '\0';
    return 0;
}

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

/* ================================================================
 * Additional POSIX.1-2024 stubs
 * ================================================================ */

#include <sys/types.h>
#include <unistd.h>
#include <stropts.h>      /* ioctl() on Solaris 7 */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>   /* SIOCATMARK on Solaris */

/* Solaris 7 declares ioctl in <unistd.h> and <stropts.h> but only when
   __EXTENSIONS__ is defined. GCC 15 cross-compile may not have it —
   provide a fallback declaration. */
extern int ioctl(int, int, ...);

#include <langinfo.h>

/* Solaris 7 net/if.h lacks struct if_nameindex — define it here */
struct if_nameindex {
    unsigned int if_index;
    char        *if_name;
};

/*
 * sockatmark — test whether socket is at out-of-band mark.
 * POSIX.1-2001. Solaris 7 has the ioctl but not the function wrapper.
 */
int
sockatmark(int socket_fd)
{
    int at_mark = 0;
    if (ioctl(socket_fd, SIOCATMARK, &at_mark) < 0)
        return -1;
    return at_mark != 0;
}

/*
 * posix_madvise — memory advisory (no-op on Solaris 7).
 * POSIX.1-2001. Solaris 7 has madvise() but not the POSIX variant.
 * The POSIX version cannot fail with EAGAIN, so a no-op is compliant.
 */
int
posix_madvise(void *address, size_t length, int advice)
{
    (void)address;
    (void)length;
    (void)advice;
    return 0;
}

/*
 * nl_langinfo_l — locale-aware langinfo.
 * POSIX.1-2008. Solaris 7 has nl_langinfo() but no per-thread variant.
 */
char *
nl_langinfo_l(int item, void *locale)
{
    (void)locale;
    return nl_langinfo(item);
}

/*
 * pthread_condattr_getclock / setclock — condition variable clock selection.
 * POSIX.1-2001. Solaris 7 pthreads don't support clock selection.
 * Stub: always report CLOCK_REALTIME, reject other clocks.
 */
#ifndef HAVE_PTHREAD_CONDATTR_GETCLOCK
int
pthread_condattr_getclock(const pthread_condattr_t *restrict attribute,
                          int *restrict clock_id)
{
    (void)attribute;
    *clock_id = 0; /* CLOCK_REALTIME */
    return 0;
}

int
pthread_condattr_setclock(pthread_condattr_t *attribute, int clock_id)
{
    /*
     * Solaris 7 pthreads have no per-clock condition variable support.
     * Accept any valid POSIX clock ID and silently succeed — conditions
     * will use CLOCK_REALTIME regardless.  libuv and other software call
     * this with CLOCK_MONOTONIC and abort if it returns EINVAL.
     */
    (void)attribute;
    (void)clock_id;
    return 0;
}
#endif

/*
 * pthread_attr_getstack / setstack — thread stack attributes.
 * POSIX.1-2001. Solaris 7 has setstacksize/getstacksize but not
 * the combined stack address + size variants.
 */
#ifndef HAVE_PTHREAD_ATTR_GETSTACK
int
pthread_attr_getstack(const pthread_attr_t *restrict attribute,
                      void **restrict stack_address,
                      size_t *restrict stack_size)
{
    /* Can't determine stack address on Solaris 7 — return defaults */
    *stack_address = NULL;
    return pthread_attr_getstacksize(attribute, stack_size);
}

int
pthread_attr_setstack(pthread_attr_t *attribute,
                      void *stack_address, size_t stack_size)
{
    /* Ignore address, just set size */
    (void)stack_address;
    return pthread_attr_setstacksize(attribute, stack_size);
}
#endif

/*
 * if_nameindex / if_freenameindex — enumerate network interfaces.
 * POSIX.1-2001. Solaris 7 has SIOCGIFCONF but not this wrapper.
 */
struct if_nameindex *
if_nameindex(void)
{
    /* Minimal implementation: return empty list.
     * A full implementation would use SIOCGIFCONF + SIOCGIFINDEX. */
    struct if_nameindex *name_index_list;
    name_index_list = (struct if_nameindex *)calloc(1, sizeof(struct if_nameindex));
    if (name_index_list) {
        name_index_list[0].if_index = 0;
        name_index_list[0].if_name = NULL;
    }
    return name_index_list;
}

void
if_freenameindex(struct if_nameindex *name_index_list)
{
    /* A full implementation would free each if_name string */
    free(name_index_list);
}

/*
 * mq_timedreceive / mq_timedsend — timed message queue operations.
 * POSIX.1-2001. Solaris 7 has basic mqueue but not timed variants.
 * Stub: return ENOSYS. Real usage is rare on Solaris 7.
 */
#include <mqueue.h>

ssize_t
mq_timedreceive(mqd_t queue_descriptor, char *message_buffer,
                 size_t message_length, unsigned int *message_priority,
                 const struct timespec *abs_timeout)
{
    (void)abs_timeout;
    /* Fall back to non-timed version */
    return mq_receive(queue_descriptor, message_buffer,
                      message_length, message_priority);
}

int
mq_timedsend(mqd_t queue_descriptor, const char *message_buffer,
              size_t message_length, unsigned int message_priority,
              const struct timespec *abs_timeout)
{
    (void)abs_timeout;
    /* Fall back to non-timed version */
    return mq_send(queue_descriptor, message_buffer,
                   message_length, message_priority);
}
