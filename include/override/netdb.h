/*
 * override/netdb.h — getaddrinfo/getnameinfo for Solaris 7
 *
 * Solaris 7's netdb.h lacks the modern getaddrinfo() family.
 * This wrapper pulls in the real header, then adds the missing APIs.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_NETDB_H
#define _SOLCOMPAT_OVERRIDE_NETDB_H

/* Pull in the real Solaris 7 /usr/include/netdb.h */
#include_next <netdb.h>

#ifdef __sun
#include <solcompat/network.h>
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_NETDB_H */
