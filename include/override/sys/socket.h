/*
 * override/sys/socket.h — IPv6 socket types for Solaris 7
 *
 * Solaris 7's sys/socket.h lacks AF_INET6 and sockaddr_storage.
 * This wrapper pulls in the real header via #include_next, then
 * adds the missing IPv6 address family and storage types from
 * solcompat/network.h.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_SYS_SOCKET_H
#define _SOLCOMPAT_OVERRIDE_SYS_SOCKET_H

/* Pull in the real Solaris 7 /usr/include/sys/socket.h */
#include_next <sys/socket.h>

/* Add AF_INET6, sockaddr_storage, and related IPv6 types */
#include <solcompat/network.h>

/*
 * POSIX.1-2001 CMSG_LEN and CMSG_SPACE macros.
 * Solaris 7 has CMSG_DATA/CMSG_FIRSTHDR/CMSG_NXTHDR but lacks these two.
 */
#ifndef CMSG_LEN
#define CMSG_LEN(data_length) \
    (_CMSG_HDR_ALIGN(sizeof(struct cmsghdr)) + (data_length))
#endif

#ifndef CMSG_SPACE
#define CMSG_SPACE(data_length) \
    (_CMSG_HDR_ALIGN(sizeof(struct cmsghdr) + (data_length)))
#endif

#endif /* _SOLCOMPAT_OVERRIDE_SYS_SOCKET_H */
