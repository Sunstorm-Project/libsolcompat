/*
 * solcompat/clock.h — Extended clock support
 *
 * CLOCK_MONOTONIC via gethrtime(), CLOCK_PROCESS_CPUTIME_ID via
 * getrusage(), clock_nanosleep() via nanosleep().
 */
#ifndef SOLCOMPAT_CLOCK_H
#define SOLCOMPAT_CLOCK_H

#include <time.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Solaris 7 only defines CLOCK_REALTIME (value 0).
 * We define additional clock IDs using values that don't conflict.
 */
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC          4   /* matches later Solaris */
#endif
#ifndef CLOCK_PROCESS_CPUTIME_ID
#define CLOCK_PROCESS_CPUTIME_ID 5
#endif
#ifndef CLOCK_THREAD_CPUTIME_ID
#define CLOCK_THREAD_CPUTIME_ID  2
#endif

/*
 * Replacement clock_gettime that handles extended clock IDs.
 * For CLOCK_REALTIME, delegates to the system implementation.
 * For CLOCK_MONOTONIC, uses gethrtime().
 * For CLOCK_PROCESS_CPUTIME_ID, uses getrusage().
 */
int solcompat_clock_gettime(clockid_t clk_id, struct timespec *tp);
#define clock_gettime solcompat_clock_gettime

int solcompat_clock_getres(clockid_t clk_id, struct timespec *res);
#define clock_getres solcompat_clock_getres

#ifndef HAVE_CLOCK_NANOSLEEP
#ifndef TIMER_ABSTIME
#define TIMER_ABSTIME 0x1
#endif
int clock_nanosleep(clockid_t clk_id, int flags,
                    const struct timespec *request,
                    struct timespec *remain);
#endif

/*
 * timegm — interpret struct tm as UTC, return time_t.
 * Inverse of gmtime(). Not in Solaris 7 (BSD/GNU extension).
 */
#ifndef HAVE_TIMEGM
time_t timegm(struct tm *tm);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_CLOCK_H */
