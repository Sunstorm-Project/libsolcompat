/*
 * override/net/if.h — if_nametoindex/if_indextoname for Solaris 7
 *
 * Solaris 7's net/if.h lacks the RFC 3493 interface index functions
 * needed for IPv6 scope IDs.  This wrapper pulls in the real header,
 * then adds declarations (implementations in libsolcompat/network.c).
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_NET_IF_H
#define _SOLCOMPAT_OVERRIDE_NET_IF_H

/* Pull in the real Solaris 7 /usr/include/net/if.h */
#include_next <net/if.h>

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- RFC 3493 interface index functions --- */
#ifndef HAVE_IF_NAMETOINDEX
unsigned int if_nametoindex(const char *ifname);
char        *if_indextoname(unsigned int ifindex, char *ifname);
#endif

/* IF_NAMESIZE — POSIX constant for max interface name */
#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

/* IFNAMSIZ — BSD/Linux compat alias */
#ifndef IFNAMSIZ
#define IFNAMSIZ IF_NAMESIZE
#endif

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_OVERRIDE_NET_IF_H */
