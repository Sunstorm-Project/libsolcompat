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
    case 3: /* process-bound Solaris CLOCK_REALTIME */
    case 0: /* original Solaris CLOCK_REALTIME */
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
    case 3:
    case 0: /* both Solaris CLOCK_REALTIME IDs use gettimeofday */
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
    long long requested, left;
    hrtime_t start;
    int absolute = flags == TIMER_ABSTIME;

    if (flags != 0 && !absolute)
        return EINVAL;
    if (clk_id == 2) /* the historical SST thread-CPU ID */
        return EINVAL;
    if (clk_id == 5) /* process CPU time cannot be measured by a wall-time sleep */
        return ENOTSUP;
    if (clk_id != 0 && clk_id != 3 && clk_id != 4)
        return EINVAL;
    if (!request || request->tv_sec < 0 ||
        request->tv_nsec < 0 || request->tv_nsec >= 1000000000L)
        return EINVAL;
    requested = (long long)request->tv_sec * 1000000000LL + request->tv_nsec;
    start = gethrtime();
    for (;;) {
        struct timeval tv;
        long long usec;
        if (absolute) {
            struct timespec now;
            if (solcompat_clock_gettime(clk_id, &now) < 0)
                return errno;
            left = requested - ((long long)now.tv_sec * 1000000000LL + now.tv_nsec);
        } else {
            left = requested - (gethrtime() - start);
        }
        if (left <= 0)
            return 0;

        /* Recheck absolute wall-clock deadlines after system clock changes.
         * Bound other chunks so the rounded timeval fits on 32-bit Solaris. */
        if (absolute && clk_id != 4 && left > 1000000000LL)
            left = 1000000000LL;
        if (left > 86400000000000LL)
            left = 86400000000000LL;
        usec = (left + 999) / 1000;
        tv.tv_sec = (time_t)(usec / 1000000);
        tv.tv_usec = (long)(usec % 1000000);
        if (select(0, NULL, NULL, NULL, &tv) < 0) {
            int error = errno;
            if (error == EINTR && !absolute && remain) {
                left = requested - (gethrtime() - start);
                if (left < 0)
                    left = 0;
                remain->tv_sec = (time_t)(left / 1000000000LL);
                remain->tv_nsec = (long)(left % 1000000000LL);
            }
            return error;
        }
    }
}

/* Convert a proleptic Gregorian date directly to days since 1970-01-01.
 * March-based years put leap day at the end of each four-year cycle. */
time_t
timegm(struct tm *tm)
{
    long long year, month, era, year_in_era, days, seconds;
    time_t result;
    struct tm normalized;

    if (!tm) {
        errno = EINVAL;
        return (time_t)-1;
    }
    year = (long long)tm->tm_year + 1900;
    month = tm->tm_mon;
    year += month / 12;
    month %= 12;
    if (month < 0) {
        month += 12;
        --year;
    }
    if (month < 2)
        --year;
    month = (month + 10) % 12;
    era = (year >= 0 ? year : year - 399) / 400;
    year_in_era = year - era * 400;
    days = era * 146097 + year_in_era * 365 + year_in_era / 4 -
        year_in_era / 100 + (153 * month + 2) / 5 + tm->tm_mday - 1 - 719468;
    seconds = days * 86400 + (long long)tm->tm_hour * 3600 +
        (long long)tm->tm_min * 60 + tm->tm_sec;
    result = (time_t)seconds;
    if ((long long)result != seconds || !gmtime_r(&result, &normalized)) {
        errno = EOVERFLOW;
        return (time_t)-1;
    }
    *tm = normalized;
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

