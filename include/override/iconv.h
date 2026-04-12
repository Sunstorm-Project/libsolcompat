/*
 * override/iconv.h — POSIX iconv signature for Solaris 7
 *
 * Solaris 7 declares iconv() with const char** for the input parameter.
 * POSIX and glibc use char** (non-const).  Modern code expects the
 * non-const version.  Provide a C++ overload.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_ICONV_H
#define _SOLCOMPAT_OVERRIDE_ICONV_H

#include_next <iconv.h>

#ifdef __sun
#ifdef __cplusplus
#include <cstddef>
static inline size_t iconv(iconv_t cd, char **inbuf, size_t *inbytesleft,
                           char **outbuf, size_t *outbytesleft) {
    return iconv(cd, const_cast<const char **>(inbuf), inbytesleft,
                 outbuf, outbytesleft);
}
#endif
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_ICONV_H */
