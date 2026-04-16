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

/*
 * wcsdup — duplicate a wide string (POSIX 2008).
 */
wchar_t *
wcsdup(const wchar_t *s)
{
    size_t byte_count = (wcslen(s) + 1) * sizeof(wchar_t);
    wchar_t *copy = (wchar_t *)malloc(byte_count);
    if (copy != NULL)
        memcpy(copy, s, byte_count);
    return copy;
}

/*
 * wcsnlen — bounded wide string length (POSIX 2008).
 */
size_t
wcsnlen(const wchar_t *s, size_t maxlen)
{
    size_t count = 0;
    while (count < maxlen && s[count] != L'\0')
        count++;
    return count;
}

/* ================================================================
 * C11 <uchar.h> — char16_t / char32_t conversions
 * ================================================================
 * C11 added mbrtoc16, c16rtomb, mbrtoc32, c32rtomb for explicit
 * UTF-16 / UTF-32 conversions.  Solaris 7 predates C11.
 *
 * On SPARC Solaris 7 ILP32, wchar_t is 'long' (4 bytes) and char32_t
 * is 'unsigned int' (4 bytes, via uint_least32_t) — both 32-bit but
 * DIFFERENT types.  The system locale determines what encoding
 * wchar_t represents; Solaris 7 uses the Process Code encoding which
 * maps UCS-4 directly to wchar_t for C/UTF-8 locales.
 *
 * These implementations delegate to mbrtowc/wcrtomb and copy values
 * through a local wchar_t buffer.  This is correct for any locale
 * whose process-code is a subset of Unicode (all Solaris stock
 * locales satisfy this).
 *
 * Why these must exist as real library symbols (not header inlines):
 * autoconf's AC_CHECK_FUNC compiles a probe that LINKS against the
 * symbol.  Without mbrtoc32 in libsolcompat.so, gawk's configure
 * detects HAVE_MBRTOC32=no.  gawk's support/regex_internal.h then
 * unconditionally includes <uchar.h> while awk.h activates a
 * `#define char32_t wchar_t` macro, and the macro rewrites uchar.h's
 * `typedef uint_least32_t char32_t` into `typedef uint_least32_t
 * wchar_t`, which collides with Solaris's `typedef long wchar_t`.
 */

typedef unsigned short  _sol_char16_t;   /* uint_least16_t */
typedef unsigned int    _sol_char32_t;   /* uint_least32_t */

size_t
mbrtoc32(_sol_char32_t *pc32, const char *s, size_t n, mbstate_t *ps)
{
    wchar_t wc;
    size_t r;

    if (s == NULL) {
        /* C11: equivalent to mbrtoc32(NULL, "", 1, ps).  Resets state. */
        s = "";
        n = 1;
        pc32 = NULL;
    }
    r = mbrtowc(&wc, s, n, ps);
    if (pc32 != NULL && r != (size_t)-1 && r != (size_t)-2)
        *pc32 = (_sol_char32_t)wc;
    return r;
}

size_t
c32rtomb(char *s, _sol_char32_t c32, mbstate_t *ps)
{
    /* C11: a NULL s means wcrtomb(buf, L'\0', ps) to reset state. */
    if (s == NULL)
        return wcrtomb(NULL, L'\0', ps);
    return wcrtomb(s, (wchar_t)c32, ps);
}

/*
 * char16_t (UTF-16) — Solaris 7's wchar_t is 32-bit process-code, so
 * we have to encode surrogate pairs ourselves for codepoints above
 * U+FFFF.  Minimal implementation covers the BMP directly and
 * delegates higher planes via surrogate state stored in mbstate_t's
 * low bits.
 */

size_t
mbrtoc16(_sol_char16_t *pc16, const char *s, size_t n, mbstate_t *ps)
{
    static mbstate_t internal_state;
    mbstate_t *state = ps ? ps : &internal_state;
    wchar_t wc;
    size_t r;

    /* Continuing from a previous call that returned a high surrogate? */
    {
        /* Probe the state bytes for a pending low-surrogate marker.
         * mbstate_t on Solaris 7 is typically 8 bytes; we piggyback
         * our low-surrogate value in the last 4 bytes (zeroed by
         * normal libc resets). */
        unsigned char *raw = (unsigned char *)state;
        _sol_char16_t pending = (_sol_char16_t)(raw[sizeof(mbstate_t) - 2]) |
                                ((_sol_char16_t)(raw[sizeof(mbstate_t) - 1]) << 8);
        if (pending != 0) {
            if (pc16)
                *pc16 = pending;
            raw[sizeof(mbstate_t) - 2] = 0;
            raw[sizeof(mbstate_t) - 1] = 0;
            return (size_t)-3;  /* continuing low surrogate, no bytes consumed */
        }
    }

    if (s == NULL) {
        s = "";
        n = 1;
        pc16 = NULL;
    }
    r = mbrtowc(&wc, s, n, state);
    if (r == (size_t)-1 || r == (size_t)-2)
        return r;

    if ((unsigned long)wc <= 0xFFFF) {
        if (pc16)
            *pc16 = (_sol_char16_t)wc;
        return r;
    }
    {
        /* Encode as UTF-16 surrogate pair.  Emit high surrogate now,
         * stash low surrogate for the next call. */
        unsigned long v = (unsigned long)wc - 0x10000;
        _sol_char16_t high = (_sol_char16_t)(0xD800 | (v >> 10));
        _sol_char16_t low = (_sol_char16_t)(0xDC00 | (v & 0x3FF));
        unsigned char *raw = (unsigned char *)state;
        raw[sizeof(mbstate_t) - 2] = (unsigned char)(low & 0xFF);
        raw[sizeof(mbstate_t) - 1] = (unsigned char)((low >> 8) & 0xFF);
        if (pc16)
            *pc16 = high;
        return r;
    }
}

size_t
c16rtomb(char *s, _sol_char16_t c16, mbstate_t *ps)
{
    static mbstate_t internal_state;
    mbstate_t *state = ps ? ps : &internal_state;

    if (s == NULL)
        return wcrtomb(NULL, L'\0', state);

    /* High surrogate — stash, emit nothing yet. */
    if (c16 >= 0xD800 && c16 <= 0xDBFF) {
        unsigned char *raw = (unsigned char *)state;
        raw[sizeof(mbstate_t) - 2] = (unsigned char)(c16 & 0xFF);
        raw[sizeof(mbstate_t) - 1] = (unsigned char)((c16 >> 8) & 0xFF);
        return 0;
    }
    /* Low surrogate — combine with stashed high. */
    if (c16 >= 0xDC00 && c16 <= 0xDFFF) {
        unsigned char *raw = (unsigned char *)state;
        _sol_char16_t high = (_sol_char16_t)(raw[sizeof(mbstate_t) - 2]) |
                             ((_sol_char16_t)(raw[sizeof(mbstate_t) - 1]) << 8);
        raw[sizeof(mbstate_t) - 2] = 0;
        raw[sizeof(mbstate_t) - 1] = 0;
        if (high >= 0xD800 && high <= 0xDBFF) {
            unsigned long codepoint = 0x10000 +
                (((unsigned long)(high - 0xD800)) << 10) +
                ((unsigned long)(c16 - 0xDC00));
            return wcrtomb(s, (wchar_t)codepoint, state);
        }
        return (size_t)-1;  /* low without high — encoding error */
    }
    return wcrtomb(s, (wchar_t)c16, state);
}
