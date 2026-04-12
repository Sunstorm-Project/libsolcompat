/*
 * override/wchar.h — Wide character extensions for Solaris 7
 *
 * Solaris 7's <wchar.h> has many wide char functions but they're
 * guarded behind __STDC__/XPG conditions that may not be active
 * during the GCC Canadian-cross build. GCC's <cwchar> does
 * 'using ::wmemset;' etc. which requires all functions to be
 * visible in the global namespace.
 *
 * This override includes the real header then declares everything
 * GCC's <cwchar> needs, using #ifndef guards to avoid conflicts
 * with declarations the system header did provide.
 *
 * Part of libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_WCHAR_H
#define _SOLCOMPAT_OVERRIDE_WCHAR_H

#include_next <wchar.h>

#ifdef __sun

#include <stddef.h>  /* size_t, wchar_t */
#include <stdio.h>   /* FILE */

#ifdef __cplusplus
extern "C" {
#endif

/* Wide memory operations — libsolcompat provides these */
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n);
int      wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n);
wchar_t *wmemcpy(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wmemmove(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n);

/* Wide string search — libsolcompat provides this */
wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle);

/* wctob — libsolcompat provides this */
int wctob(wint_t c);

/* btowc — Solaris 7 libc has it but may not declare under all conditions */
#ifndef _SOLCOMPAT_BTOWC
#define _SOLCOMPAT_BTOWC
wint_t btowc(int c);
#endif

/* Multibyte ↔ wide conversion — in Solaris 7 libc but may be hidden */
#ifndef _SOLCOMPAT_MBRTOWC
#define _SOLCOMPAT_MBRTOWC
size_t mbrlen(const char *s, size_t n, mbstate_t *ps);
size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);
int    mbsinit(const mbstate_t *ps);
size_t mbsrtowcs(wchar_t *dest, const char **src, size_t len, mbstate_t *ps);
size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps);
size_t wcsrtombs(char *dest, const wchar_t **src, size_t len, mbstate_t *ps);
#endif

/* Wide I/O — in Solaris 7 libc but may be hidden */
#ifndef _SOLCOMPAT_FWPRINTF
#define _SOLCOMPAT_FWPRINTF
int fwprintf(FILE *stream, const wchar_t *fmt, ...);
int fwscanf(FILE *stream, const wchar_t *fmt, ...);
int wprintf(const wchar_t *fmt, ...);
int wscanf(const wchar_t *fmt, ...);
int swprintf(wchar_t *s, size_t n, const wchar_t *fmt, ...);
int swscanf(const wchar_t *s, const wchar_t *fmt, ...);
int fwide(FILE *stream, int mode);
#endif

/* Variadic wide I/O */
#ifndef _SOLCOMPAT_VFWPRINTF
#define _SOLCOMPAT_VFWPRINTF
#include <stdarg.h>
int vfwprintf(FILE *stream, const wchar_t *fmt, va_list ap);
int vwprintf(const wchar_t *fmt, va_list ap);
int vswprintf(wchar_t *s, size_t n, const wchar_t *fmt, va_list ap);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_WCHAR_H */
