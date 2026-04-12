/*
 * wchar.c — Wide character functions for Solaris 7
 *
 * Solaris 7 has basic wchar support but lacks the C99 wmem* family,
 * wcsstr, wcrtomb/wcsrtombs, and wctob.
 */

#include <wchar.h>
#include <string.h>
#include <stdlib.h>

/* ================================================================
 * Wide memory operations
 * ================================================================ */

wchar_t *
wmemchr(const wchar_t *s, wchar_t c, size_t n)
{
    while (n-- > 0) {
        if (*s == c)
            return (wchar_t *)s;
        s++;
    }
    return NULL;
}

int
wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
    while (n-- > 0) {
        if (*s1 != *s2)
            return (*s1 > *s2) ? 1 : -1;
        s1++;
        s2++;
    }
    return 0;
}

wchar_t *
wmemcpy(wchar_t *dest, const wchar_t *src, size_t n)
{
    return (wchar_t *)memcpy(dest, src, n * sizeof(wchar_t));
}

wchar_t *
wmemmove(wchar_t *dest, const wchar_t *src, size_t n)
{
    return (wchar_t *)memmove(dest, src, n * sizeof(wchar_t));
}

wchar_t *
wmemset(wchar_t *s, wchar_t c, size_t n)
{
    wchar_t *p = s;
    while (n-- > 0)
        *p++ = c;
    return s;
}

/* ================================================================
 * Wide string search
 * ================================================================ */

wchar_t *
wcsstr(const wchar_t *haystack, const wchar_t *needle)
{
    size_t needle_len;
    if (*needle == L'\0')
        return (wchar_t *)haystack;
    needle_len = wcslen(needle);
    while (*haystack != L'\0') {
        if (*haystack == *needle && wcsncmp(haystack, needle, needle_len) == 0)
            return (wchar_t *)haystack;
        haystack++;
    }
    return NULL;
}

/* wcrtomb, wcsrtombs, mbsrtowcs: provided by Solaris 7's libc.
 * Do NOT reimplement — the system versions handle the locale correctly. */

int
wctob(wint_t c)
{
    if (c >= 0 && c < 256)
        return (int)c;
    return -1;  /* EOF */
}
