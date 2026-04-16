/*
 * override/time.h — Wrap system <time.h> with CLOCK_MONOTONIC support
 *
 * Solaris 7's <time.h> lacks CLOCK_MONOTONIC, CLOCK_PROCESS_CPUTIME_ID,
 * and other POSIX clock IDs that modern software expects.  This override
 * includes the system header, then pulls in solcompat/clock.h which
 * defines the missing constants and provides replacement clock_gettime()
 * that implements CLOCK_MONOTONIC via gethrtime().
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_TIME_H
#define _SOLCOMPAT_OVERRIDE_TIME_H

/* Include the real system <time.h> */
#include_next <time.h>

#ifdef __sun
#include <solcompat/clock.h>

/*
 * Solaris 7's time.h only declares gmtime_r and localtime_r when
 * _REENTRANT is defined. Many packages (OpenSSL, etc.) don't define
 * _REENTRANT but still use these functions. Ensure they're always
 * declared.
 *
 * Note: ctime_r and asctime_r POSIX signatures are provided by
 * Solaris 7's time.h when _POSIX_PTHREAD_SEMANTICS is defined (the
 * SST build adds this to COMMON_CPPFLAGS in common.sh). No override
 * needed.
 */
#ifndef _REENTRANT
#ifdef __cplusplus
extern "C" {
#endif
extern struct tm *gmtime_r(const time_t *, struct tm *);
extern struct tm *localtime_r(const time_t *, struct tm *);
#ifdef __cplusplus
}
#endif
#endif /* !_REENTRANT */
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_TIME_H */
