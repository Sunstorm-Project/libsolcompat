/*
 * override/arpa/inet.h — inet_ntop/inet_pton for Solaris 7
 *
 * Solaris 7's arpa/inet.h declares inet_pton but NOT inet_ntop.
 * This wrapper pulls in the real header, then adds the missing
 * declarations (both functions are implemented in libsolcompat).
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_ARPA_INET_H
#define _SOLCOMPAT_OVERRIDE_ARPA_INET_H

/* Pull in the real Solaris 7 /usr/include/arpa/inet.h */
#include_next <arpa/inet.h>

/* Add inet_ntop declaration and IPv6 types it depends on */
#include <solcompat/network.h>

#endif /* _SOLCOMPAT_OVERRIDE_ARPA_INET_H */
