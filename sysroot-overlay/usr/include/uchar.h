/*
 * uchar.h — C11 Unicode character types for Solaris 7
 *
 * C11 added char16_t and char32_t plus mbrtoc16/c32rtomb/etc. for
 * UTF-16/UTF-32 conversion. Solaris 7 predates C11 by a decade.
 * Declarations only — implementations can wrap libiconv (SSTliconv)
 * when a consumer actually calls them.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SOLCOMPAT_UCHAR_H
#define _SOLCOMPAT_UCHAR_H

#include <stddef.h>
#include <wchar.h>     /* mbstate_t */
#include <stdint.h>    /* uint_least16_t, uint_least32_t */

typedef uint_least16_t char16_t;
typedef uint_least32_t char32_t;

#ifdef __cplusplus
extern "C" {
#endif

size_t mbrtoc16(char16_t *pc16, const char *s, size_t n, mbstate_t *ps);
size_t c16rtomb(char *s, char16_t c16, mbstate_t *ps);
size_t mbrtoc32(char32_t *pc32, const char *s, size_t n, mbstate_t *ps);
size_t c32rtomb(char *s, char32_t c32, mbstate_t *ps);

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_UCHAR_H */
