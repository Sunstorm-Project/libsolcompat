/*
 * solcompat/network.h — Missing networking APIs
 *
 * getaddrinfo, freeaddrinfo, gai_strerror, getnameinfo,
 * inet_ntop, inet_pton, struct addrinfo, struct sockaddr_storage,
 * getifaddrs, freeifaddrs, IPv6 type stubs (AF_INET6, sockaddr_in6)
 */
#ifndef SOLCOMPAT_NETWORK_H
#define SOLCOMPAT_NETWORK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Address length constants --- */
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN  16
#endif
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

/* --- IPv6 address family and protocol constants --- */
/*
 * Solaris 7 has experimental IPv6 support and may define AF_INET6 in
 * <sys/socket.h>.  If the sysroot headers don't provide it (or if the
 * installation lacks the IPv6 optional package), we define the constants
 * and structures here so code referencing IPv6 will compile.
 *
 * At runtime on Solaris 7, socket(AF_INET6, ...) may return EAFNOSUPPORT
 * if the IPv6 kernel module isn't loaded, but the calling code (e.g.
 * libcody) handles socket creation failures gracefully.
 */
#ifndef AF_INET6
#define AF_INET6 26   /* Solaris AF_INET6 value */
#endif
#ifndef PF_INET6
#define PF_INET6 AF_INET6
#endif
#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6 41
#endif

/* --- struct in6_addr / sockaddr_in6 --- */
/*
 * Guard on s6_addr being defined — it is always a macro (required by
 * POSIX/RFC 2553) when struct in6_addr exists. If the system already
 * provides IPv6 types, s6_addr will be defined and we skip everything.
 * On Solaris 7 sysroots that lack IPv6 headers, s6_addr won't exist.
 */
#ifndef s6_addr
struct in6_addr {
    union {
        unsigned char  _S6_u8[16];
        unsigned short _S6_u16[8];
        unsigned int   _S6_u32[4];
    } _S6_un;
};
#define s6_addr   _S6_un._S6_u8
#define s6_addr16 _S6_un._S6_u16
#define s6_addr32 _S6_un._S6_u32

#ifndef IN6ADDR_ANY_INIT
#define IN6ADDR_ANY_INIT      { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }
#endif
#ifndef IN6ADDR_LOOPBACK_INIT
#define IN6ADDR_LOOPBACK_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }
#endif

struct sockaddr_in6 {
    sa_family_t     sin6_family;    /* AF_INET6 */
    unsigned short  sin6_port;      /* transport layer port # (network byte order) */
    unsigned int    sin6_flowinfo;  /* IPv6 flow info */
    struct in6_addr sin6_addr;      /* IPv6 address */
    unsigned int    sin6_scope_id;  /* scope zone index */
};
#endif /* !s6_addr */

/* --- sockaddr_storage --- */
#ifndef _SS_MAXSIZE
#define _SS_MAXSIZE    128
#define _SS_ALIGNSIZE  (sizeof(int64_t))
#define _SS_PAD1SIZE   (_SS_ALIGNSIZE - sizeof(sa_family_t))
#define _SS_PAD2SIZE   (_SS_MAXSIZE - sizeof(sa_family_t) - _SS_PAD1SIZE - _SS_ALIGNSIZE)

struct sockaddr_storage {
    sa_family_t  ss_family;
    char         _ss_pad1[_SS_PAD1SIZE];
    int64_t      _ss_align;
    char         _ss_pad2[_SS_PAD2SIZE];
};
#endif

/* --- struct addrinfo --- */
#ifndef AI_PASSIVE
#define AI_PASSIVE     0x0001
#define AI_CANONNAME   0x0002
#define AI_NUMERICHOST 0x0004
#define AI_NUMERICSERV 0x0008
#define AI_ADDRCONFIG  0x0020

#define NI_MAXHOST     1025
#define NI_MAXSERV     32
#define NI_NUMERICHOST 0x0001
#define NI_NUMERICSERV 0x0002
#define NI_NOFQDN      0x0004
#define NI_NAMEREQD    0x0008
#define NI_DGRAM       0x0010

#define EAI_AGAIN      2
#define EAI_BADFLAGS   3
#define EAI_FAIL       4
#define EAI_FAMILY     5
#define EAI_MEMORY     6
#define EAI_NONAME     8
#define EAI_SERVICE    9
#define EAI_SOCKTYPE   10
#define EAI_SYSTEM     11
#define EAI_OVERFLOW   14

struct addrinfo {
    int              ai_flags;
    int              ai_family;
    int              ai_socktype;
    int              ai_protocol;
    socklen_t        ai_addrlen;
    char            *ai_canonname;
    struct sockaddr *ai_addr;
    struct addrinfo *ai_next;
};

int  getaddrinfo(const char *node, const char *service,
                 const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo(struct addrinfo *res);
const char *gai_strerror(int errcode);
int  getnameinfo(const struct sockaddr *sa, socklen_t salen,
                 char *host, socklen_t hostlen,
                 char *serv, socklen_t servlen, int flags);
#endif /* AI_PASSIVE */

/* --- inet_ntop / inet_pton --- */
#ifndef HAVE_INET_NTOP
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
#endif
#ifndef HAVE_INET_PTON
int inet_pton(int af, const char *src, void *dst);
#endif

/* --- getifaddrs --- */
#ifndef HAVE_GETIFADDRS
struct ifaddrs {
    struct ifaddrs  *ifa_next;
    char            *ifa_name;
    unsigned int     ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
    union {
        struct sockaddr *ifu_broadaddr;
        struct sockaddr *ifu_dstaddr;
    } ifa_ifu;
    void            *ifa_data;
};
#ifndef ifa_broadaddr
#define ifa_broadaddr ifa_ifu.ifu_broadaddr
#endif
#ifndef ifa_dstaddr
#define ifa_dstaddr   ifa_ifu.ifu_dstaddr
#endif

int  getifaddrs(struct ifaddrs **ifap);
void freeifaddrs(struct ifaddrs *ifa);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_NETWORK_H */
