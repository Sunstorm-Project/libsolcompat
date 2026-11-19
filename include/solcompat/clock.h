/*
 * solcompat/clock.h — Extended clock support
 *
 * CLOCK_MONOTONIC via gethrtime(), CLOCK_PROCESS_CPUTIME_ID via
 * getrusage(), clock_nanosleep() via nanosleep().
 */
#ifndef SOLCOMPAT_CLOCK_H
#define SOLCOMPAT_CLOCK_H

/* Use sys headers directly to avoid circular include with override/time.h.
   clockid_t is in <sys/types.h>, struct timespec in <sys/time.h>. */
#include <sys/types.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Solaris uses realtime ID 0 or 3 depending on the caller's feature macros.
 * Preserve the existing SST IDs for compatibility.
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

#ifndef TIMER_ABSTIME
#define TIMER_ABSTIME 0x1
#endif
int clock_nanosleep(clockid_t clk_id, int flags,
                    const struct timespec *request,
                    struct timespec *remain);

/*
 * timegm — interpret struct tm as UTC, return time_t.
 * Inverse of gmtime(). Not in Solaris 7 (BSD/GNU extension).
 */
time_t timegm(struct tm *tm);

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_CLOCK_H */
