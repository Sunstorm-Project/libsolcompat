/*
 * Part of libsolcompat sysroot — provides getifaddrs/freeifaddrs for Solaris 7
 *
 * Solaris 7 has no <ifaddrs.h>.  This header provides struct ifaddrs
 * and the getifaddrs/freeifaddrs functions backed by libsolcompat's
 * SIOCGIFCONF-based implementation.
 */
#ifndef _SOLCOMPAT_SYSROOT_IFADDRS_H
#define _SOLCOMPAT_SYSROOT_IFADDRS_H

#include <sys/types.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_SYSROOT_IFADDRS_H */
