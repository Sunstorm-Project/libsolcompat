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

#ifndef LC_GLOBAL_LOCALE
#define LC_GLOBAL_LOCALE ((locale_t)-1)
#endif

#ifndef LC_ALL_MASK
#define LC_COLLATE_MASK  (1 << LC_COLLATE)
#define LC_CTYPE_MASK    (1 << LC_CTYPE)
#define LC_MESSAGES_MASK (1 << LC_MESSAGES)
#define LC_MONETARY_MASK (1 << LC_MONETARY)
#define LC_NUMERIC_MASK  (1 << LC_NUMERIC)
#define LC_TIME_MASK     (1 << LC_TIME)
#define LC_ALL_MASK      (LC_COLLATE_MASK | LC_CTYPE_MASK | LC_MESSAGES_MASK | \
                          LC_MONETARY_MASK | LC_NUMERIC_MASK | LC_TIME_MASK)
#endif

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

/* pthread_attr_getstack/setstack (POSIX.1-2001) */
int pthread_attr_getstack(const pthread_attr_t *, void **, size_t *);
int pthread_attr_setstack(pthread_attr_t *, void *, size_t);

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_STUBS_H */
