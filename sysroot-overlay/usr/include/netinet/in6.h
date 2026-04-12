/*
 * Part of libsolcompat sysroot — provides <netinet/in6.h> redirect for Solaris 7
 *
 * Some software includes <netinet/in6.h> directly for IPv6 types.
 * On Solaris 7, IPv6 types are provided by libsolcompat through
 * <netinet/in.h> (via sysroot-prep appends).  Redirect here.
 */
#ifndef _SOLCOMPAT_SYSROOT_NETINET_IN6_H
#define _SOLCOMPAT_SYSROOT_NETINET_IN6_H

#include <netinet/in.h>

#endif /* _SOLCOMPAT_SYSROOT_NETINET_IN6_H */
