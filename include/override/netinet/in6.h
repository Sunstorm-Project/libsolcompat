/*
 * override/netinet/in6.h — IPv6 types for Solaris 7
 *
 * Solaris 7 does not ship netinet/in6.h at all (IPv6 types were added
 * in Solaris 8).  Some modern software includes this header directly.
 * This provides all IPv6 types via solcompat/network.h.
 *
 * Note: no #include_next since the real file doesn't exist.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_NETINET_IN6_H
#define _SOLCOMPAT_OVERRIDE_NETINET_IN6_H

/* Pull in netinet/in.h (which chains through our override to add IPv6) */
#include <netinet/in.h>

/* Everything else comes from solcompat/network.h via the in.h override */

#endif /* _SOLCOMPAT_OVERRIDE_NETINET_IN6_H */
