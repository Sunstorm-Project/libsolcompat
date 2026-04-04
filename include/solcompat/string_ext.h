/*
 * solcompat/string_ext.h — Missing string functions
 *
 * strndup, strnlen, strlcpy, strlcat, strcasestr, memmem,
 * strsep, stpcpy, stpncpy, strchrnul, memrchr
 */
#ifndef SOLCOMPAT_STRING_EXT_H
#define SOLCOMPAT_STRING_EXT_H

#include <string.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

char *strndup(const char *s, size_t n);

size_t strnlen(const char *s, size_t maxlen);

size_t strlcpy(char *dst, const char *src, size_t size);

size_t strlcat(char *dst, const char *src, size_t size);

char *strcasestr(const char *haystack, const char *needle);

void *memmem(const void *haystack, size_t haystacklen,
             const void *needle, size_t needlelen);

char *strsep(char **stringp, const char *delim);

char *stpcpy(char *dst, const char *src);

char *stpncpy(char *dst, const char *src, size_t n);

char *strchrnul(const char *s, int c);

void *memrchr(const void *s, int c, size_t n);

/* --- strsignal --- */
char *strsignal(int signum);

/*
 * strerror_r — GNU-compatible wrapper (returns char*, not int).
 * Solaris 7 has strerror_r with POSIX (int) signature.  Only override
 * when the build hasn't already opted to use the system version.
 */
#ifndef HAVE_STRERROR_R
char *solcompat_strerror_r(int errnum, char *buf, size_t buflen);
#define strerror_r solcompat_strerror_r
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_STRING_EXT_H */
