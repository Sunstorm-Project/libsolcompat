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

#ifndef HAVE_STRNDUP
char *strndup(const char *s, size_t n);
#endif

#ifndef HAVE_STRNLEN
size_t strnlen(const char *s, size_t maxlen);
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t size);
#endif

#ifndef HAVE_STRLCAT
size_t strlcat(char *dst, const char *src, size_t size);
#endif

#ifndef HAVE_STRCASESTR
char *strcasestr(const char *haystack, const char *needle);
#endif

#ifndef HAVE_MEMMEM
void *memmem(const void *haystack, size_t haystacklen,
             const void *needle, size_t needlelen);
#endif

#ifndef HAVE_STRSEP
char *strsep(char **stringp, const char *delim);
#endif

#ifndef HAVE_STPCPY
char *stpcpy(char *dst, const char *src);
#endif

#ifndef HAVE_STPNCPY
char *stpncpy(char *dst, const char *src, size_t n);
#endif

#ifndef HAVE_STRCHRNUL
char *strchrnul(const char *s, int c);
#endif

#ifndef HAVE_MEMRCHR
void *memrchr(const void *s, int c, size_t n);
#endif

/* --- strsignal --- */
#ifndef HAVE_STRSIGNAL
char *strsignal(int signum);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_STRING_EXT_H */
