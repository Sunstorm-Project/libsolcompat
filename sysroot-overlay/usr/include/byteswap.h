/*
 * byteswap.h — GNU byte-swap primitives for Solaris 7
 *
 * Solaris 7 lacks <byteswap.h>. bash/coreutils/gettext include it
 * unconditionally. libsolcompat provides the bswap_16/32/64 via
 * compiler builtins; packages linking against this get the same
 * ABI as glibc.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SOLCOMPAT_BYTESWAP_H
#define _SOLCOMPAT_BYTESWAP_H

#define bswap_16(x) __builtin_bswap16(x)
#define bswap_32(x) __builtin_bswap32(x)
#define bswap_64(x) __builtin_bswap64(x)

/* Legacy aliases some packages still use */
#define __bswap_16(x) bswap_16(x)
#define __bswap_32(x) bswap_32(x)
#define __bswap_64(x) bswap_64(x)

#endif /* _SOLCOMPAT_BYTESWAP_H */
