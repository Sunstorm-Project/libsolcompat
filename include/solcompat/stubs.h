/*
 * solcompat/stubs.h — Stubbed locale and threading helpers
 *
 * pthread_setname_np, newlocale/uselocale/freelocale
 */
#ifndef SOLCOMPAT_STUBS_H
#define SOLCOMPAT_STUBS_H

#include <pthread.h>
#include <locale.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- pthread_setname_np --- */
int pthread_setname_np(pthread_t thread, const char *name);

/* --- Per-thread locale (POSIX 2008) --- */

/*
 * locale_t, LC_GLOBAL_LOCALE, and the LC_*_MASK constants must ALWAYS
 * be defined, regardless of whether the function stubs are provided.
 *
 * The problem: configure tests find our uselocale()/newlocale() stubs and
 * set HAVE_USELOCALE=1, then expect locale_t and LC_GLOBAL_LOCALE to come
 * from the system's <locale.h>.  Solaris 7's <locale.h> has neither, so
 * gnulib code that includes only <locale.h> fails with "unknown type name
 * 'locale_t'".  Defining the types unconditionally here — pulled in via
 * the override locale.h — fixes that.
 *
 * __sun guard: canadian-cross GCC's build stage compiles helper binaries
 * (libcpp, libiberty, the generators) as x86_64-linux translation units.
 * Those compiles may still pick up our override/ctype.h via -isystem if
 * the canadian toolchain's include path is mis-ordered, and on glibc
 * locale_t is a `struct __locale_struct *` — a conflicting typedef.
 * Gating on __sun keeps the libsolcompat locale types confined to
 * actual Solaris compiles where they belong.
 */
#ifdef __sun
#ifndef _SOLCOMPAT_LOCALE_T
#define _SOLCOMPAT_LOCALE_T
typedef struct _sol_locale_s *locale_t;
#endif

/*
 * locale_t, LC_GLOBAL_LOCALE, and LC_*_MASK are authoritatively defined
 * in sysroot-prep/locale.h.append, which installs into the sysroot's
 * /usr/include/locale.h. The previous duplicate definitions here
 * collided-by-accident with those — the literal `(1 << 0)` in the
 * append and the `(1 << LC_CTYPE)` here happened to agree because
 * Solaris 7 ships LC_CTYPE=0, but a sysroot patch that reordered the
 * LC_* constants would silently desync the two sets. Single source of
 * truth now: sysroot-prep. #include <locale.h> gets the masks.
 *
 * #include <locale.h> above already pulls them in; no fallbacks here.
 */

/* Function declarations */
locale_t newlocale(int category_mask, const char *locale, locale_t base);
locale_t uselocale(locale_t newloc);
void     freelocale(locale_t locobj);
locale_t duplocale(locale_t locobj);
#endif /* __sun */

/* --- Additional POSIX.1-2024 stubs --- */

/* sockatmark (POSIX.1-2001) */
int sockatmark(int);

/* posix_madvise (POSIX.1-2001, no-op on Solaris 7) */
int posix_madvise(void *, size_t, int);
#ifndef POSIX_MADV_NORMAL
#define POSIX_MADV_NORMAL    0
#define POSIX_MADV_SEQUENTIAL 1
#define POSIX_MADV_RANDOM    2
#define POSIX_MADV_WILLNEED  3
#define POSIX_MADV_DONTNEED  4
#endif

/* nl_langinfo_l (POSIX.1-2008) — only available on Solaris compiles */
#ifdef __sun
char *nl_langinfo_l(int, locale_t);
#endif

/* pthread_condattr_getclock/setclock (POSIX.1-2001) */
int pthread_condattr_getclock(const pthread_condattr_t *, int *);
int pthread_condattr_setclock(pthread_condattr_t *, int);

/* solcompat_pthread_cond_timedwait_monotonic — explicit monotonic-timeout
 * variant for callers that need CLOCK_MONOTONIC semantics. Solaris 7
 * condvars are REALTIME-only; this wrapper translates the abs_timeout
 * from monotonic to realtime at call time.  Prefer this over
 * pthread_cond_timedwait when wall-clock-jump resistance matters. */
struct timespec;
int solcompat_pthread_cond_timedwait_monotonic(pthread_cond_t *,
                                               pthread_mutex_t *,
                                               const struct timespec *);

/* pthread_attr_getstack/setstack (POSIX.1-2001) */
int pthread_attr_getstack(const pthread_attr_t *, void **, size_t *);
int pthread_attr_setstack(pthread_attr_t *, void *, size_t);

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_STUBS_H */
