/*
 * override/netinet/in.h — IPv6 structures for Solaris 7
 *
 * Solaris 7's netinet/in.h lacks struct in6_addr, struct sockaddr_in6,
 * IPPROTO_IPV6, and all IPv6 socket options.  This wrapper pulls in the
 * real header, then adds the missing types and constants.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_NETINET_IN_H
#define _SOLCOMPAT_OVERRIDE_NETINET_IN_H

/* Pull in the real Solaris 7 /usr/include/netinet/in.h */
#include_next <netinet/in.h>

#ifdef __sun
#include <solcompat/network.h>
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_NETINET_IN_H */
