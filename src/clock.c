/*
 * clock.c — Extended clock support for Solaris 7
 *
 * CLOCK_MONOTONIC via gethrtime() (available since Solaris 2.3)
 * CLOCK_PROCESS_CPUTIME_ID via /proc/self/usage or getrusage
 * clock_nanosleep via nanosleep with adjustment
 * timegm — inverse of gmtime (UTC mktime), not in Solaris 7
 */

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/select.h>  /* select(0, NULL, NULL, NULL, &tv) — nanosleep-free sleep */
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Forward declarations for functions implemented in stdlib.c —
   Solaris 7 stdlib.h doesn't declare these. */
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);

/* gethrtime() is a Solaris builtin — returns hrtime_t (nanoseconds) */
extern hrtime_t gethrtime(void);

/* Unhide our names (macros from headers may have redefined these) */
#undef clock_gettime
#undef clock_getres

/* Weak aliases so packages that call clock_gettime/clock_getres directly
   (libpng's contrib/libtests/timepng, gnulib probes that don't honor our
   header macros, etc.) resolve against libsolcompat.so without needing
   -lrt. Solaris 7 puts the real symbols in librt (libposix4) which many
   third-party configures fail to detect. The weak attribute lets librt
   override us if it's explicitly linked. */
int clock_gettime(clockid_t clk_id, struct timespec *tp)
    __attribute__((weak, alias("solcompat_clock_gettime")));
int clock_getres(clockid_t clk_id, struct timespec *res)
    __attribute__((weak, alias("solcompat_clock_getres")));

int
solcompat_clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    switch (clk_id) {
    case 0: /* CLOCK_REALTIME */
        {
            struct timeval tv;
            if (gettimeofday(&tv, NULL) < 0)
                return -1;
            tp->tv_sec = tv.tv_sec;
            tp->tv_nsec = tv.tv_usec * 1000;
            return 0;
        }

    case 4: /* CLOCK_MONOTONIC */
        {
            hrtime_t ns = gethrtime();
            tp->tv_sec = (time_t)(ns / 1000000000LL);
            tp->tv_nsec = (long)(ns % 1000000000LL);
            return 0;
        }

    case 5: /* CLOCK_PROCESS_CPUTIME_ID */
        {
            struct rusage ru;
            if (getrusage(RUSAGE_SELF, &ru) < 0)
                return -1;
            /* user + system time */
            tp->tv_sec = ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
            tp->tv_nsec = (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) * 1000;
            if (tp->tv_nsec >= 1000000000L) {
                tp->tv_sec++;
                tp->tv_nsec -= 1000000000L;
            }
            return 0;
        }

    case 2: /* CLOCK_THREAD_CPUTIME_ID — approximate with process */
        {
            struct rusage ru;
            if (getrusage(RUSAGE_SELF, &ru) < 0)
                return -1;
            tp->tv_sec = ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
            tp->tv_nsec = (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) * 1000;
            if (tp->tv_nsec >= 1000000000L) {
                tp->tv_sec++;
                tp->tv_nsec -= 1000000000L;
            }
            return 0;
        }

    default:
        errno = EINVAL;
        return -1;
    }
}

int
solcompat_clock_getres(clockid_t clk_id, struct timespec *res)
{
    switch (clk_id) {
    case 0: /* CLOCK_REALTIME — microsecond via gettimeofday */
        if (res) {
            res->tv_sec = 0;
            res->tv_nsec = 1000; /* 1 microsecond */
        }
        return 0;

    case 4: /* CLOCK_MONOTONIC — gethrtime is nanosecond */
        if (res) {
            res->tv_sec = 0;
            res->tv_nsec = 1; /* 1 nanosecond (nominal) */
        }
        return 0;

    case 5: /* CLOCK_PROCESS_CPUTIME_ID */
    case 2: /* CLOCK_THREAD_CPUTIME_ID */
        if (res) {
            res->tv_sec = 0;
            res->tv_nsec = 1000; /* microsecond (from rusage) */
        }
        return 0;

    default:
        errno = EINVAL;
        return -1;
    }
}

int
clock_nanosleep(clockid_t clk_id, int flags,
                const struct timespec *request,
                struct timespec *remain)
{
    struct timespec req;

    if (flags & 0x1 /* TIMER_ABSTIME */) {
        /* Convert absolute time to relative */
        struct timespec now;
        if (solcompat_clock_gettime(clk_id, &now) < 0)
            return errno;

        req.tv_sec = request->tv_sec - now.tv_sec;
        req.tv_nsec = request->tv_nsec - now.tv_nsec;
        if (req.tv_nsec < 0) {
            req.tv_sec--;
            req.tv_nsec += 1000000000L;
        }
        if (req.tv_sec < 0)
            return 0; /* Already past */
    } else {
        req = *request;
    }

    /* Use select() instead of nanosleep() so the object file doesn't
       carry an unresolved nanosleep@@SUNW_0.7 reference that requires
       -lrt at final link. select() lives in Solaris 7 libc;
       microsecond precision is sufficient for the callers we see. */
    {
        struct timeval tv_req, tv_start, tv_end;
        int rc;
        tv_req.tv_sec = req.tv_sec;
        tv_req.tv_usec = req.tv_nsec / 1000;
        /* Round up sub-microsecond remainder so we never under-sleep. */
        if (req.tv_nsec % 1000 != 0 && tv_req.tv_usec < 999999)
            tv_req.tv_usec++;
        gettimeofday(&tv_start, NULL);
        rc = select(0, NULL, NULL, NULL, &tv_req);
        if (rc < 0) {
            /* EINTR: compute real remaining time so callers (gnulib's
               rpl_nanosleep, etc.) can resume correctly. */
            if (remain && errno == EINTR) {
                long long elapsed_us, requested_us, remain_us;
                gettimeofday(&tv_end, NULL);
                elapsed_us = (long long)(tv_end.tv_sec - tv_start.tv_sec) * 1000000LL
                           + (tv_end.tv_usec - tv_start.tv_usec);
                requested_us = (long long)req.tv_sec * 1000000LL
                             + (req.tv_nsec / 1000);
                remain_us = requested_us - elapsed_us;
                if (remain_us < 0) remain_us = 0;
                remain->tv_sec = (time_t)(remain_us / 1000000LL);
                remain->tv_nsec = (long)((remain_us % 1000000LL) * 1000);
            }
            return errno;
        }
        if (remain) {
            remain->tv_sec = 0;
            remain->tv_nsec = 0;
        }
    }

    return 0;
}

/*
 * timegm — interpret struct tm as UTC and return time_t.
 *
 * Solaris 7 has mktime (local time) but not timegm (UTC).
 * We use the portable setenv("TZ","UTC")/mktime/restore approach.
 *
 * Not thread-safe, but neither is mktime on Solaris 7.
 * Single-CPU QEMU makes this a non-issue in practice.
 */
time_t
timegm(struct tm *tm)
{
    time_t result;
    char *saved_tz;
    char *tz_env;

    tz_env = getenv("TZ");
    saved_tz = tz_env ? strdup(tz_env) : NULL;

    setenv("TZ", "UTC", 1);
    tzset();

    result = mktime(tm);

    if (saved_tz != NULL) {
        setenv("TZ", saved_tz, 1);
        free(saved_tz);
    } else {
        unsetenv("TZ");
    }
    tzset();

    return result;
}

/*
 * timespec_get — C11 §7.27.2.5 high-resolution clock.
 * Returns the base on success, 0 on failure.
 * Only TIME_UTC (base=1) is required by C11.
 */
int
_solcompat_timespec_get(struct timespec *ts, int base)
{
    if (base != 1)  /* TIME_UTC = 1 */
        return 0;
    if (solcompat_clock_gettime(0 /* CLOCK_REALTIME */, ts) != 0)
        return 0;
    return base;
}
