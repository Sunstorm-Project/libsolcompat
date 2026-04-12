/*
 * override/wchar.h — Wide character extensions for Solaris 7
 *
 * Solaris 7's <wchar.h> lacks many C99/POSIX wide character functions
 * that GCC's <cwchar> expects: wmemchr, wmemcmp, wmemcpy, wmemmove,
 * wmemset, wprintf, wscanf, wcsstr, wcsrtombs, wcrtomb, wctob, etc.
 *
 * This wrapper includes the real header then declares the missing
 * functions (implemented in libsolcompat's wchar.c).
 *
 * Part of libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_WCHAR_H
#define _SOLCOMPAT_OVERRIDE_WCHAR_H

#include_next <wchar.h>

#ifdef __sun

#include <stddef.h>  /* size_t, wchar_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Wide memory operations (C99) */
#ifndef _SOLCOMPAT_WMEM_DECLARED
#define _SOLCOMPAT_WMEM_DECLARED
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n);
int      wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n);
wchar_t *wmemcpy(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wmemmove(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n);
#endif

/* Wide string search (C99) */
#ifndef _SOLCOMPAT_WCSSTR_DECLARED
#define _SOLCOMPAT_WCSSTR_DECLARED
wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle);
#endif

/* Wide character ↔ multibyte conversion (C99) */
#ifndef _SOLCOMPAT_WCRTOMB_DECLARED
#define _SOLCOMPAT_WCRTOMB_DECLARED
typedef struct { int __state; } mbstate_t_solcompat;
size_t  wcrtomb(char *s, wchar_t wc, void *ps);
size_t  wcsrtombs(char *dest, const wchar_t **src, size_t len, void *ps);
int     wctob(wint_t c);
size_t  mbsrtowcs(wchar_t *dest, const char **src, size_t len, void *ps);
#endif

/* Wide I/O (C99) — stub declarations for compilation only.
 * The Canadian-cross GCC compiles cc1 with these in scope but the
 * SPARC binary will use Solaris 7's own wprintf at runtime. */
#ifndef _SOLCOMPAT_WPRINTF_DECLARED
#define _SOLCOMPAT_WPRINTF_DECLARED
int wprintf(const wchar_t *fmt, ...);
int wscanf(const wchar_t *fmt, ...);
int swprintf(wchar_t *s, size_t n, const wchar_t *fmt, ...);
int swscanf(const wchar_t *s, const wchar_t *fmt, ...);
int fwprintf(void *stream, const wchar_t *fmt, ...);
int fwscanf(void *stream, const wchar_t *fmt, ...);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_WCHAR_H */
