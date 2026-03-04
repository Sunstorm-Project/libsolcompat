/*
 * string.c — Missing string functions for Solaris 7
 *
 * strndup, strnlen, strlcpy, strlcat, strcasestr, memmem,
 * strsep, stpcpy, stpncpy, strchrnul, memrchr,
 * strtoimax, strtoumax, strerror_r (GNU variant)
 */

#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* Solaris: strcasecmp */
#include <ctype.h>
#include <errno.h>

/* Forward declaration — defined in snprintf.c */
extern int solcompat_snprintf(char *, size_t, const char *, ...);

char *
strndup(const char *s, size_t n)
{
    size_t len = 0;
    char *dup;

    while (len < n && s[len] != '\0')
        len++;

    dup = (char *)malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len);
        dup[len] = '\0';
    }
    return dup;
}

size_t
strnlen(const char *s, size_t maxlen)
{
    size_t len = 0;
    while (len < maxlen && s[len] != '\0')
        len++;
    return len;
}

size_t
strlcpy(char *dst, const char *src, size_t size)
{
    size_t srclen = strlen(src);

    if (size > 0) {
        size_t copylen = (srclen >= size) ? size - 1 : srclen;
        memcpy(dst, src, copylen);
        dst[copylen] = '\0';
    }
    return srclen;
}

size_t
strlcat(char *dst, const char *src, size_t size)
{
    size_t dstlen = strnlen(dst, size);
    size_t srclen = strlen(src);

    if (dstlen == size)
        return size + srclen;

    if (srclen < size - dstlen) {
        memcpy(dst + dstlen, src, srclen + 1);
    } else {
        memcpy(dst + dstlen, src, size - dstlen - 1);
        dst[size - 1] = '\0';
    }
    return dstlen + srclen;
}

char *
strcasestr(const char *haystack, const char *needle)
{
    size_t nlen;

    if (!needle[0])
        return (char *)haystack;

    nlen = strlen(needle);
    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, nlen) == 0)
            return (char *)haystack;
    }
    return NULL;
}

void *
memmem(const void *haystack, size_t haystacklen,
       const void *needle, size_t needlelen)
{
    const unsigned char *h = (const unsigned char *)haystack;
    const unsigned char *n = (const unsigned char *)needle;
    size_t i;

    if (needlelen == 0)
        return (void *)haystack;
    if (needlelen > haystacklen)
        return NULL;

    for (i = 0; i <= haystacklen - needlelen; i++) {
        if (h[i] == n[0] && memcmp(h + i, n, needlelen) == 0)
            return (void *)(h + i);
    }
    return NULL;
}

char *
strsep(char **stringp, const char *delim)
{
    char *start = *stringp;
    char *p;

    if (start == NULL)
        return NULL;

    p = strpbrk(start, delim);
    if (p) {
        *p = '\0';
        *stringp = p + 1;
    } else {
        *stringp = NULL;
    }
    return start;
}

char *
stpcpy(char *dst, const char *src)
{
    while ((*dst = *src) != '\0') {
        dst++;
        src++;
    }
    return dst;
}

char *
stpncpy(char *dst, const char *src, size_t n)
{
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; i++)
        dst[i] = src[i];
    for (; i < n; i++)
        dst[i] = '\0';

    /* Return pointer to the NUL, or to dst+n if no NUL was written */
    return (i < n) ? &dst[i] : &dst[n];
}

char *
strchrnul(const char *s, int c)
{
    while (*s && *s != (char)c)
        s++;
    return (char *)s;
}

void *
memrchr(const void *s, int c, size_t n)
{
    const unsigned char *p = (const unsigned char *)s + n;
    while (p != (const unsigned char *)s) {
        --p;
        if (*p == (unsigned char)c)
            return (void *)p;
    }
    return NULL;
}

long long
strtoimax(const char *nptr, char **endptr, int base)
{
    return strtoll(nptr, endptr, base);
}

unsigned long long
strtoumax(const char *nptr, char **endptr, int base)
{
    return strtoull(nptr, endptr, base);
}

/*
 * GNU-compatible strerror_r: returns a pointer to the error string.
 * The POSIX version returns int — we provide the GNU version since
 * that's what most GNU software expects.
 */
char *
solcompat_strerror_r(int errnum, char *buf, size_t buflen)
{
    const char *msg = strerror(errnum);
    if (msg) {
        strlcpy(buf, msg, buflen);
        return buf;
    }
    solcompat_snprintf(buf, buflen, "Unknown error %d", errnum);
    return buf;
}
