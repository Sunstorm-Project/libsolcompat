/*
 * solcompat/network.h — Missing networking APIs & IPv6 types for Solaris 7
 *
 * getaddrinfo, freeaddrinfo, gai_strerror, getnameinfo,
 * inet_ntop, inet_pton, struct addrinfo, struct sockaddr_storage,
 * getifaddrs, freeifaddrs, if_nametoindex, if_indextoname,
 * AF_INET6, struct in6_addr, struct sockaddr_in6, IN6_IS_ADDR_* macros,
 * IPv6 socket options (IPV6_V6ONLY, etc.), in6addr_any, in6addr_loopback
 */
#ifndef SOLCOMPAT_NETWORK_H
#define SOLCOMPAT_NETWORK_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

/* --- Global IPv6 address constants (defined in network.c) --- */
extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;

/* --- IN6_IS_ADDR_* classification macros (RFC 3513 / RFC 4291) --- */
#ifndef IN6_IS_ADDR_UNSPECIFIED
#define IN6_IS_ADDR_UNSPECIFIED(a) \
    (((const unsigned int *)(a))[0] == 0 && \
     ((const unsigned int *)(a))[1] == 0 && \
     ((const unsigned int *)(a))[2] == 0 && \
     ((const unsigned int *)(a))[3] == 0)
#endif

#ifndef IN6_IS_ADDR_LOOPBACK
#define IN6_IS_ADDR_LOOPBACK(a) \
    (((const unsigned int *)(a))[0] == 0 && \
     ((const unsigned int *)(a))[1] == 0 && \
     ((const unsigned int *)(a))[2] == 0 && \
     ((const unsigned int *)(a))[3] == htonl(1))
#endif

#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(a) \
    (((const unsigned char *)(a))[0] == 0xff)
#endif

#ifndef IN6_IS_ADDR_LINKLOCAL
#define IN6_IS_ADDR_LINKLOCAL(a) \
    ((((const unsigned char *)(a))[0] == 0xfe) && \
     (((const unsigned char *)(a))[1] & 0xc0) == 0x80)
#endif

#ifndef IN6_IS_ADDR_SITELOCAL
#define IN6_IS_ADDR_SITELOCAL(a) \
    ((((const unsigned char *)(a))[0] == 0xfe) && \
     (((const unsigned char *)(a))[1] & 0xc0) == 0xc0)
#endif

#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED(a) \
    (((const unsigned int *)(a))[0] == 0 && \
     ((const unsigned int *)(a))[1] == 0 && \
     ((const unsigned int *)(a))[2] == htonl(0x0000ffff))
#endif

#ifndef IN6_IS_ADDR_V4COMPAT
#define IN6_IS_ADDR_V4COMPAT(a) \
    (((const unsigned int *)(a))[0] == 0 && \
     ((const unsigned int *)(a))[1] == 0 && \
     ((const unsigned int *)(a))[2] == 0 && \
     ((const unsigned int *)(a))[3] != 0 && \
     ((const unsigned int *)(a))[3] != htonl(1))
#endif

#ifndef IN6_IS_ADDR_MC_NODELOCAL
#define IN6_IS_ADDR_MC_NODELOCAL(a) \
    (IN6_IS_ADDR_MULTICAST(a) && \
     (((const unsigned char *)(a))[1] & 0x0f) == 0x01)
#endif

#ifndef IN6_IS_ADDR_MC_LINKLOCAL
#define IN6_IS_ADDR_MC_LINKLOCAL(a) \
    (IN6_IS_ADDR_MULTICAST(a) && \
     (((const unsigned char *)(a))[1] & 0x0f) == 0x02)
#endif

#ifndef IN6_IS_ADDR_MC_SITELOCAL
#define IN6_IS_ADDR_MC_SITELOCAL(a) \
    (IN6_IS_ADDR_MULTICAST(a) && \
     (((const unsigned char *)(a))[1] & 0x0f) == 0x05)
#endif

#ifndef IN6_IS_ADDR_MC_ORGLOCAL
#define IN6_IS_ADDR_MC_ORGLOCAL(a) \
    (IN6_IS_ADDR_MULTICAST(a) && \
     (((const unsigned char *)(a))[1] & 0x0f) == 0x08)
#endif

#ifndef IN6_IS_ADDR_MC_GLOBAL
#define IN6_IS_ADDR_MC_GLOBAL(a) \
    (IN6_IS_ADDR_MULTICAST(a) && \
     (((const unsigned char *)(a))[1] & 0x0f) == 0x0e)
#endif

/* --- IN6_ARE_ADDR_EQUAL (RFC 3493) --- */
/*
 * Compares two struct in6_addr for equality.  Not provided by Solaris 7.
 * Used by Tcl, Python, and other networking code.
 */
#ifndef IN6_ARE_ADDR_EQUAL
#include <string.h>
#define IN6_ARE_ADDR_EQUAL(a, b) \
    (memcmp((a), (b), sizeof(struct in6_addr)) == 0)
#endif

#endif /* !s6_addr */

/* --- IPv6 socket options (IPPROTO_IPV6 level) --- */
/*
 * These are needed by modern software (.NET, Node.js, etc.) for
 * dual-stack and IPv6 multicast configuration.  Values match
 * Solaris 8+ definitions.
 */
#ifndef IPV6_UNICAST_HOPS
#define IPV6_UNICAST_HOPS    7
#endif
#ifndef IPV6_MULTICAST_IF
#define IPV6_MULTICAST_IF    8
#endif
#ifndef IPV6_MULTICAST_HOPS
#define IPV6_MULTICAST_HOPS  9
#endif
#ifndef IPV6_MULTICAST_LOOP
#define IPV6_MULTICAST_LOOP  10
#endif
#ifndef IPV6_JOIN_GROUP
#define IPV6_JOIN_GROUP      11
#endif
#ifndef IPV6_LEAVE_GROUP
#define IPV6_LEAVE_GROUP     12
#endif
/* BSD compat aliases */
#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP  IPV6_JOIN_GROUP
#endif
#ifndef IPV6_DROP_MEMBERSHIP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif
#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY          39
#endif
#ifndef IPV6_PKTINFO
#define IPV6_PKTINFO         11  /* control message type */
#endif
#ifndef IPV6_RECVPKTINFO
#define IPV6_RECVPKTINFO     18
#endif
#ifndef IPV6_HOPLIMIT
#define IPV6_HOPLIMIT        12
#endif
#ifndef IPV6_RECVHOPLIMIT
#define IPV6_RECVHOPLIMIT    19
#endif
#ifndef IPV6_RECVTCLASS
#define IPV6_RECVTCLASS      25
#endif
#ifndef IPV6_TCLASS
#define IPV6_TCLASS          26
#endif

/* struct in6_pktinfo -- used with IPV6_PKTINFO ancillary data */
#ifndef _SOLCOMPAT_IN6_PKTINFO
#define _SOLCOMPAT_IN6_PKTINFO
struct in6_pktinfo {
    struct in6_addr ipi6_addr;      /* source/destination IPv6 address */
    unsigned int    ipi6_ifindex;   /* interface index */
};
#endif

/* --- IPv6 multicast group request --- */
#ifndef _SOLCOMPAT_IPV6_MREQ
#define _SOLCOMPAT_IPV6_MREQ
struct ipv6_mreq {
    struct in6_addr ipv6mr_multiaddr;  /* IPv6 multicast address */
    unsigned int    ipv6mr_interface;  /* interface index */
};
#endif

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

/* --- Source-specific multicast (RFC 3678 / Solaris 10+) ---
 *
 * Solaris 7 lacks source-specific multicast.  Modern networking code
 * (libuv, etc.) references these structures unconditionally on __sun.
 * Provide compile-time stubs so the source builds; runtime calls would
 * fail with EOPNOTSUPP since Solaris 7's kernel has no support.
 *
 * Defined after sockaddr_storage because group_req/group_source_req
 * embed it.
 */
#ifndef IP_ADD_SOURCE_MEMBERSHIP
#define IP_ADD_SOURCE_MEMBERSHIP   39
#endif
#ifndef IP_DROP_SOURCE_MEMBERSHIP
#define IP_DROP_SOURCE_MEMBERSHIP  40
#endif
#ifndef IP_BLOCK_SOURCE
#define IP_BLOCK_SOURCE            41
#endif
#ifndef IP_UNBLOCK_SOURCE
#define IP_UNBLOCK_SOURCE          42
#endif

#ifndef _SOLCOMPAT_IP_MREQ_SOURCE
#define _SOLCOMPAT_IP_MREQ_SOURCE
struct ip_mreq_source {
    struct in_addr imr_multiaddr;   /* IP multicast address of group */
    struct in_addr imr_interface;   /* local IP address of interface */
    struct in_addr imr_sourceaddr;  /* IP address of source */
};
#endif

#ifndef MCAST_JOIN_GROUP
#define MCAST_JOIN_GROUP           41
#endif
#ifndef MCAST_LEAVE_GROUP
#define MCAST_LEAVE_GROUP          42
#endif
#ifndef MCAST_JOIN_SOURCE_GROUP
#define MCAST_JOIN_SOURCE_GROUP    46
#endif
#ifndef MCAST_LEAVE_SOURCE_GROUP
#define MCAST_LEAVE_SOURCE_GROUP   47
#endif
#ifndef MCAST_BLOCK_SOURCE
#define MCAST_BLOCK_SOURCE         43
#endif
#ifndef MCAST_UNBLOCK_SOURCE
#define MCAST_UNBLOCK_SOURCE       44
#endif

#ifndef _SOLCOMPAT_GROUP_REQ
#define _SOLCOMPAT_GROUP_REQ
struct group_req {
    uint32_t                gr_interface; /* interface index */
    struct sockaddr_storage gr_group;     /* group address */
};

struct group_source_req {
    uint32_t                gsr_interface; /* interface index */
    struct sockaddr_storage gsr_group;     /* group address */
    struct sockaddr_storage gsr_source;    /* source address */
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
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
int inet_pton(int af, const char *src, void *dst);

/* --- getifaddrs --- */
/*
 * Struct ifaddrs is always needed — even when HAVE_GETIFADDRS is set
 * (meaning configure detected our libsolcompat getifaddrs function),
 * the struct definition isn't provided by Solaris 7's system headers.
 */
#ifndef _SOLCOMPAT_STRUCT_IFADDRS
#define _SOLCOMPAT_STRUCT_IFADDRS
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
#endif /* _SOLCOMPAT_STRUCT_IFADDRS */

int  getifaddrs(struct ifaddrs **ifap);
void freeifaddrs(struct ifaddrs *ifa);

/* --- if_nametoindex / if_indextoname (RFC 3493) --- */
/*
 * Needed for IPv6 scope IDs and modern socket code.
 * Implementation uses SIOCGIFINDEX/SIOCGIFCONF ioctls.
 */
unsigned int if_nametoindex(const char *ifname);
char        *if_indextoname(unsigned int ifindex, char *ifname);

/* IF_NAMESIZE — POSIX constant for max interface name length */
#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif
#ifndef IFNAMSIZ
#define IFNAMSIZ IF_NAMESIZE
#endif

/* --- SOCK_CLOEXEC / SOCK_NONBLOCK flags for accept4 --- */
#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC  0x80000
#endif
#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK 0x800
#endif

/* --- accept4 --- */
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_NETWORK_H */
