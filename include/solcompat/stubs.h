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
#ifndef HAVE_PTHREAD_SETNAME_NP
int pthread_setname_np(pthread_t thread, const char *name);
#endif

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
 */
#ifndef _SOLCOMPAT_LOCALE_T_DEFINED
#define _SOLCOMPAT_LOCALE_T_DEFINED
typedef void *locale_t;
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

/* Function declarations — only when the system doesn't provide them */
#ifndef HAVE_USELOCALE
locale_t newlocale(int category_mask, const char *locale, locale_t base);
locale_t uselocale(locale_t newloc);
void     freelocale(locale_t locobj);
locale_t duplocale(locale_t locobj);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_STUBS_H */
