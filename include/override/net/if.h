/*
 * override/net/if.h — Missing ifr_mtu for Solaris 7
 *
 * Solaris 7 uses SIOCGIFMTU but returns the value through ifr_metric.
 * Modern code expects ifr_mtu. Define it as an alias.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_NET_IF_H
#define _SOLCOMPAT_OVERRIDE_NET_IF_H

#include_next <net/if.h>

#ifndef ifr_mtu
#define ifr_mtu ifr_metric
#endif

#endif /* _SOLCOMPAT_OVERRIDE_NET_IF_H */
