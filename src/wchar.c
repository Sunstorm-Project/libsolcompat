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

/* ================================================================
 * Wide character ↔ multibyte conversion
 * Minimal implementations assuming Solaris 7's default C locale
 * where wchar_t values < 256 map directly to bytes.
 * ================================================================ */

size_t
wcrtomb(char *s, wchar_t wc, mbstate_t *ps)
{
    (void)ps;
    if (s == NULL)
        return 1;
    if (wc < 256) {
        *s = (char)wc;
        return 1;
    }
    return (size_t)-1;
}

size_t
wcsrtombs(char *dest, const wchar_t **src, size_t len, mbstate_t *ps)
{
    size_t count = 0;
    const wchar_t *s = *src;
    (void)ps;

    while (len > 0) {
        if (*s == L'\0') {
            if (dest)
                *dest = '\0';
            *src = NULL;
            return count;
        }
        if (*s >= 256) {
            return (size_t)-1;
        }
        if (dest) {
            *dest++ = (char)*s;
        }
        s++;
        count++;
        len--;
    }
    *src = s;
    return count;
}

size_t
mbsrtowcs(wchar_t *dest, const char **src, size_t len, mbstate_t *ps)
{
    size_t count = 0;
    const char *s = *src;
    (void)ps;

    while (len > 0) {
        if (*s == '\0') {
            if (dest)
                *dest = L'\0';
            *src = NULL;
            return count;
        }
        if (dest) {
            *dest++ = (wchar_t)(unsigned char)*s;
        }
        s++;
        count++;
        len--;
    }
    *src = s;
    return count;
}

int
wctob(wint_t c)
{
    if (c >= 0 && c < 256)
        return (int)c;
    return -1;  /* EOF */
}
