/*
 * xlocale.h — POSIX 2008 per-thread locale for Solaris 7
 *
 * Solaris 7 has locale.h but not <xlocale.h>. Modern packages
 * (bash, coreutils, gettext, glib, openal-soft, openssl) include
 * this header expecting locale_t, newlocale, freelocale, uselocale,
 * duplocale, and the _l variants of string/number conversion
 * functions. On Solaris 7 these don't exist; we declare prototypes
 * with ENOTSUP-returning stubs or locale-ignoring fallbacks provided
 * by libsolcompat src/stdlib.c.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SOLCOMPAT_XLOCALE_H
#define _SOLCOMPAT_XLOCALE_H

#include <locale.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SOLCOMPAT_LOCALE_T
#define _SOLCOMPAT_LOCALE_T
typedef struct _sol_locale_s *locale_t;
#endif

#ifndef LC_GLOBAL_LOCALE
#define LC_GLOBAL_LOCALE ((locale_t)-1)
#endif

#ifndef LC_ALL_MASK
#define LC_CTYPE_MASK    (1 << 0)
#define LC_NUMERIC_MASK  (1 << 1)
#define LC_TIME_MASK     (1 << 2)
#define LC_COLLATE_MASK  (1 << 3)
#define LC_MONETARY_MASK (1 << 4)
#define LC_MESSAGES_MASK (1 << 5)
#define LC_ALL_MASK      ((1 << 6) - 1)
#endif

locale_t newlocale(int category_mask, const char *locale, locale_t base);
locale_t duplocale(locale_t loc);
void     freelocale(locale_t loc);
locale_t uselocale(locale_t newloc);

/* _l variants — libsolcompat implementations ignore the locale and
 * fall through to the C-locale behavior. Sufficient for packages
 * that only call these to escape locale sensitivity. */
long   strtol_l(const char *s, char **end, int base, locale_t loc);
long long strtoll_l(const char *s, char **end, int base, locale_t loc);
unsigned long strtoul_l(const char *s, char **end, int base, locale_t loc);
unsigned long long strtoull_l(const char *s, char **end, int base, locale_t loc);
double strtod_l(const char *s, char **end, locale_t loc);
float  strtof_l(const char *s, char **end, locale_t loc);

int    isalnum_l(int c, locale_t loc);
int    isalpha_l(int c, locale_t loc);
int    isblank_l(int c, locale_t loc);
int    iscntrl_l(int c, locale_t loc);
int    isdigit_l(int c, locale_t loc);
int    isgraph_l(int c, locale_t loc);
int    islower_l(int c, locale_t loc);
int    isprint_l(int c, locale_t loc);
int    ispunct_l(int c, locale_t loc);
int    isspace_l(int c, locale_t loc);
int    isupper_l(int c, locale_t loc);
int    isxdigit_l(int c, locale_t loc);
int    tolower_l(int c, locale_t loc);
int    toupper_l(int c, locale_t loc);

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_XLOCALE_H */
