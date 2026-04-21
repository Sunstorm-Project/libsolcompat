/*
 * sys/byteorder.h -- Solaris 7 overlay with LE_N and BE_N macros
 *
 * Solaris 10+ <sys/byteorder.h> defines LE_16, LE_32, LE_64, BE_16, BE_32,
 * BE_64 as host-to-byte-order conversion macros.  Solaris 7 only ships
 * htonl, htons, ntohl, ntohs.  Packages written for modern Solaris (or
 * ported from userspace OpenZFS or Illumos) that go via the __sun__ branch
 * of their own endianness detection (e.g. Python _hacl or krml) expect the
 * richer macro set and fail to compile without it.
 *
 * This overlay is a drop-in replacement that preserves the Solaris 7
 * htonl, htons, ntohl, ntohs contract and adds the missing macros.  SPARC
 * Solaris 7 is always big-endian, so the BE_N macros are identity and the
 * LE_N macros byte-swap.
 *
 * Part of libsolcompat -- https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SYS_BYTEORDER_H
#define _SYS_BYTEORDER_H

#include <sys/isa_defs.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Host <-> network byte order (preserved from Solaris 7 sys/byteorder.h).
 * Solaris 7 SPARC is big-endian, so these are identity.
 */
#if defined(_BIG_ENDIAN) && !defined(ntohl) && !defined(lint)
#define ntohl(x)        (x)
#define ntohs(x)        (x)
#define htonl(x)        (x)
#define htons(x)        (x)
#elif !defined(ntohl)
#ifndef _IN_PORT_T
#define _IN_PORT_T
typedef uint16_t in_port_t;
#endif
#ifndef _IN_ADDR_T
#define _IN_ADDR_T
typedef uint32_t in_addr_t;
#endif
extern uint32_t htonl(uint32_t);
extern uint16_t htons(uint16_t);
extern uint32_t ntohl(uint32_t);
extern uint16_t ntohs(uint16_t);
#endif

/*
 * Byte-swap primitives and LE_N / BE_N host-conversion macros (Solaris 10+).
 * Uses GCC __builtin_bswap16/32/64 so the compiler picks efficient sequences
 * (single instruction on SPARCv9, inline shift-and-mask on v8).
 */
#define BSWAP_16(x)     ((uint16_t)__builtin_bswap16((uint16_t)(x)))
#define BSWAP_32(x)     ((uint32_t)__builtin_bswap32((uint32_t)(x)))
#define BSWAP_64(x)     ((uint64_t)__builtin_bswap64((uint64_t)(x)))

#ifdef _BIG_ENDIAN
#define BE_16(x)        ((uint16_t)(x))
#define BE_32(x)        ((uint32_t)(x))
#define BE_64(x)        ((uint64_t)(x))
#define LE_16(x)        BSWAP_16(x)
#define LE_32(x)        BSWAP_32(x)
#define LE_64(x)        BSWAP_64(x)
#else
#define BE_16(x)        BSWAP_16(x)
#define BE_32(x)        BSWAP_32(x)
#define BE_64(x)        BSWAP_64(x)
#define LE_16(x)        ((uint16_t)(x))
#define LE_32(x)        ((uint32_t)(x))
#define LE_64(x)        ((uint64_t)(x))
#endif

#ifdef __cplusplus
}
#endif

#endif /* _SYS_BYTEORDER_H */
