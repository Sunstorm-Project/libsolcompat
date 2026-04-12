/*
 * override/wchar.h — Wide character extensions for Solaris 7
 *
 * Solaris 7's <wchar.h> provides basic wide char support including
 * mbsrtowcs, wcsrtombs, wcrtomb, fwprintf, fwscanf, etc. But it
 * LACKS the C99 wmem* family, wcsstr, and wctob that GCC's <cwchar>
 * needs.
 *
 * Only declare the MISSING functions — do NOT redeclare functions
 * that the system header already provides, as GCC's include-fixed
 * copies them and redeclaration causes type conflicts.
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

/* Wide memory operations — NOT in Solaris 7's <wchar.h> */
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n);
int      wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n);
wchar_t *wmemcpy(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wmemmove(wchar_t *dest, const wchar_t *src, size_t n);
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n);

/* Wide string search — NOT in Solaris 7's <wchar.h> */
wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle);

/* wctob — NOT in Solaris 7's <wchar.h> */
int wctob(wint_t c);

/* wprintf/wscanf family — NOT in Solaris 7's <wchar.h>.
 * Note: fwprintf/fwscanf ARE in the system header, don't redeclare. */
#ifndef _SOLCOMPAT_WPRINTF_DECLARED
#define _SOLCOMPAT_WPRINTF_DECLARED
int wprintf(const wchar_t *fmt, ...);
int wscanf(const wchar_t *fmt, ...);
int swprintf(wchar_t *s, size_t n, const wchar_t *fmt, ...);
int swscanf(const wchar_t *s, const wchar_t *fmt, ...);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_WCHAR_H */
