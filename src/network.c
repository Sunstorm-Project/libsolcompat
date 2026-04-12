/*
 * network.c — Missing networking APIs for Solaris 7
 *
 * getaddrinfo/freeaddrinfo/gai_strerror/getnameinfo, 
 * inet_ntop/inet_pton, getifaddrs/freeifaddrs
 *
 * getaddrinfo implementation is based on gethostbyname/getservbyname
 * which are available on Solaris 7.  IPv6 is not supported (Solaris 7
 * has experimental IPv6 but we don't rely on it).
 */

#include <stdarg.h>   /* __gnuc_va_list used by Solaris 7 stdio.h prototypes */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

/* SOCK_CLOEXEC / SOCK_NONBLOCK — Linux/BSD socket flags */
#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC  0x80000
#endif
#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK 0x800
#endif

/* Unhide — we define the real functions here */
#undef getaddrinfo
#undef freeaddrinfo
#undef gai_strerror
#undef getnameinfo

/* Use our fixed snprintf */
extern int solcompat_snprintf(char *, size_t, const char *, ...);

/* Pull in our own struct definitions */
#include "solcompat/network.h"

/*
 * Global IPv6 address constants (RFC 2553 / RFC 3493).
 * in6addr_any is all-zeros (::), in6addr_loopback is ::1.
 */
#ifndef s6_addr
/* If we defined in6_addr ourselves, provide the globals */
const struct in6_addr in6addr_any      = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
#else
/* System has in6_addr but might not export globals — weak symbols */
#pragma weak in6addr_any
#pragma weak in6addr_loopback
const struct in6_addr in6addr_any      = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
#endif

/*
 * getaddrinfo — resolve hostname and service to socket addresses.
 * This is an IPv4-only implementation using gethostbyname/getservbyname.
 */
int
getaddrinfo(const char *node, const char *service,
            const struct addrinfo *hints, struct addrinfo **res)
{
    struct hostent *he = NULL;
    struct servent *se = NULL;
    struct addrinfo *head = NULL, **tail = &head;
    struct sockaddr_in *sin;
    int port = 0;
    int socktype = 0;
    int protocol = 0;
    int flags = 0;
    int family = AF_UNSPEC;
    int i;

    if (!node && !service)
        return EAI_NONAME;

    if (hints) {
        flags = hints->ai_flags;
        family = hints->ai_family;
        socktype = hints->ai_socktype;
        protocol = hints->ai_protocol;

        if (family != AF_UNSPEC && family != AF_INET && family != AF_INET6)
            return EAI_FAMILY;
    }

    /*
     * IPv6: We cannot resolve AAAA records with gethostbyname().
     * If the caller specifically asked for AF_INET6, we return
     * EAI_NONAME (no addresses found) which is the honest answer
     * on a system without IPv6 DNS resolution.  Callers like
     * libcody handle this gracefully.
     */
    if (family == AF_INET6)
        return EAI_NONAME;

    /* Resolve service */
    if (service) {
        char *endptr;
        long portnum = strtol(service, &endptr, 10);
        if (*endptr == '\0' && portnum >= 0 && portnum <= 65535) {
            port = htons((unsigned short)portnum);
        } else {
            /* AI_NUMERICSERV: reject non-numeric service names */
            if (flags & AI_NUMERICSERV)
                return EAI_NONAME;
            const char *proto = NULL;
            if (socktype == SOCK_STREAM) proto = "tcp";
            else if (socktype == SOCK_DGRAM) proto = "udp";
            se = getservbyname(service, proto);
            if (!se)
                return EAI_SERVICE;
            port = se->s_port;
        }
    }

    /* Resolve hostname */
    if (node) {
        if (flags & AI_NUMERICHOST) {
            struct in_addr addr;
            if (inet_aton(node, &addr) == 0)
                return EAI_NONAME;

            /* Create single result */
            struct addrinfo *ai = (struct addrinfo *)calloc(1, sizeof(*ai) + sizeof(struct sockaddr_in));
            if (!ai)
                return EAI_MEMORY;

            sin = (struct sockaddr_in *)(ai + 1);
            sin->sin_family = AF_INET;
            sin->sin_port = port;
            sin->sin_addr = addr;

            ai->ai_family = AF_INET;
            ai->ai_socktype = socktype ? socktype : SOCK_STREAM;
            ai->ai_protocol = protocol;
            ai->ai_addrlen = sizeof(struct sockaddr_in);
            ai->ai_addr = (struct sockaddr *)sin;
            ai->ai_next = NULL;

            *res = ai;
            return 0;
        }

        he = gethostbyname(node);
        if (!he) {
            switch (h_errno) {
            case HOST_NOT_FOUND: return EAI_NONAME;
            case TRY_AGAIN:      return EAI_AGAIN;
            case NO_RECOVERY:    return EAI_FAIL;
            default:             return EAI_NONAME;
            }
        }
    }

    if (he) {
        /* Create a result for each address */
        for (i = 0; he->h_addr_list[i]; i++) {
            struct addrinfo *ai = (struct addrinfo *)calloc(1, sizeof(*ai) + sizeof(struct sockaddr_in));
            if (!ai) {
                freeaddrinfo(head);
                return EAI_MEMORY;
            }

            sin = (struct sockaddr_in *)(ai + 1);
            sin->sin_family = AF_INET;
            sin->sin_port = port;
            memcpy(&sin->sin_addr, he->h_addr_list[i], (size_t)he->h_length);

            ai->ai_family = AF_INET;
            ai->ai_socktype = socktype ? socktype : SOCK_STREAM;
            ai->ai_protocol = protocol;
            ai->ai_addrlen = sizeof(struct sockaddr_in);
            ai->ai_addr = (struct sockaddr *)sin;
            ai->ai_next = NULL;

            if ((flags & AI_CANONNAME) && i == 0 && he->h_name) {
                ai->ai_canonname = strdup(he->h_name);
            }

            *tail = ai;
            tail = &ai->ai_next;
        }
    } else {
        /* No node specified — passive or loopback */
        struct addrinfo *ai = (struct addrinfo *)calloc(1, sizeof(*ai) + sizeof(struct sockaddr_in));
        if (!ai)
            return EAI_MEMORY;

        sin = (struct sockaddr_in *)(ai + 1);
        sin->sin_family = AF_INET;
        sin->sin_port = port;
        sin->sin_addr.s_addr = (flags & AI_PASSIVE) ? INADDR_ANY : htonl(INADDR_LOOPBACK);

        ai->ai_family = AF_INET;
        ai->ai_socktype = socktype ? socktype : SOCK_STREAM;
        ai->ai_protocol = protocol;
        ai->ai_addrlen = sizeof(struct sockaddr_in);
        ai->ai_addr = (struct sockaddr *)sin;
        ai->ai_next = NULL;

        *tail = ai;
    }

    if (!head)
        return EAI_NONAME;

    /* If no socktype specified, duplicate results for TCP and UDP */
    if (hints && hints->ai_socktype == 0) {
        struct addrinfo *p;
        struct addrinfo *udp_head = NULL, **udp_tail = &udp_head;
        for (p = head; p; p = p->ai_next) {
            struct addrinfo *dup = (struct addrinfo *)calloc(1, sizeof(*dup) + sizeof(struct sockaddr_in));
            if (!dup) break;
            memcpy(dup, p, sizeof(*dup));
            sin = (struct sockaddr_in *)(dup + 1);
            memcpy(sin, p->ai_addr, sizeof(struct sockaddr_in));
            dup->ai_addr = (struct sockaddr *)sin;
            dup->ai_socktype = SOCK_DGRAM;
            dup->ai_protocol = IPPROTO_UDP;
            dup->ai_canonname = NULL;
            dup->ai_next = NULL;
            *udp_tail = dup;
            udp_tail = &dup->ai_next;
        }
        /* Append UDP results after TCP */
        for (p = head; p->ai_next; p = p->ai_next)
            ;
        p->ai_next = udp_head;
    }

    *res = head;
    return 0;
}

void
freeaddrinfo(struct addrinfo *res)
{
    struct addrinfo *next;
    while (res) {
        next = res->ai_next;
        if (res->ai_canonname)
            free(res->ai_canonname);
        free(res);
        res = next;
    }
}

const char *
gai_strerror(int errcode)
{
    switch (errcode) {
    case 0:            return "Success";
    case EAI_AGAIN:    return "Temporary failure in name resolution";
    case EAI_BADFLAGS: return "Invalid value for ai_flags";
    case EAI_FAIL:     return "Non-recoverable failure in name resolution";
    case EAI_FAMILY:   return "ai_family not supported";
    case EAI_MEMORY:   return "Memory allocation failure";
    case EAI_NONAME:   return "Name or service not known";
    case EAI_SERVICE:  return "Servname not supported for ai_socktype";
    case EAI_SOCKTYPE: return "ai_socktype not supported";
    case EAI_SYSTEM:   return "System error";
    default:           return "Unknown error";
    }
}

int
getnameinfo(const struct sockaddr *sa, socklen_t salen,
            char *host, socklen_t hostlen,
            char *serv, socklen_t servlen, int flags)
{
    struct hostent *he;
    struct servent *se;
    unsigned short port;

    if (sa->sa_family == AF_INET) {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
        port = sin->sin_port;

        if (host && hostlen > 0) {
            if (flags & NI_NUMERICHOST) {
                char *ip = inet_ntoa(sin->sin_addr);
                if (strlen(ip) >= hostlen)
                    return EAI_OVERFLOW;
                strcpy(host, ip);
            } else {
                he = gethostbyaddr((const char *)&sin->sin_addr,
                                   sizeof(sin->sin_addr), AF_INET);
                if (he) {
                    if (strlen(he->h_name) >= hostlen)
                        return EAI_OVERFLOW;
                    strcpy(host, he->h_name);
                } else {
                    if (flags & NI_NAMEREQD)
                        return EAI_NONAME;
                    char *ip = inet_ntoa(sin->sin_addr);
                    if (strlen(ip) >= hostlen)
                        return EAI_OVERFLOW;
                    strcpy(host, ip);
                }
            }
        }
    } else if (sa->sa_family == AF_INET6) {
        const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
        port = sin6->sin6_port;

        if (host && hostlen > 0) {
            /* Always numeric for IPv6 — no reverse DNS on Solaris 7 */
            if (!inet_ntop(AF_INET6, &sin6->sin6_addr, host, hostlen))
                return EAI_OVERFLOW;
        }
    } else {
        return EAI_FAMILY;
    }

    if (serv && servlen > 0) {
        if (flags & NI_NUMERICSERV) {
            solcompat_snprintf(serv, servlen, "%d", ntohs(port));
        } else {
            const char *proto = (flags & NI_DGRAM) ? "udp" : "tcp";
            se = getservbyport(port, proto);
            if (se) {
                if (strlen(se->s_name) >= servlen)
                    return EAI_OVERFLOW;
                strcpy(serv, se->s_name);
            } else {
                solcompat_snprintf(serv, servlen, "%d", ntohs(port));
            }
        }
    }

    return 0;
}

/*
 * inet_ntop — convert binary address to presentation string
 */
const char *
inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (af == AF_INET) {
        const unsigned char *p = (const unsigned char *)src;
        int n = solcompat_snprintf(dst, size, "%u.%u.%u.%u",
                                   p[0], p[1], p[2], p[3]);
        if (n < 0 || (socklen_t)n >= size) {
            errno = ENOSPC;
            return NULL;
        }
        return dst;
    }
    if (af == AF_INET6) {
        const unsigned char *p = (const unsigned char *)src;
        unsigned int words[8];
        int i;
        /* Best run of consecutive zero-words for :: compression */
        int best_start = -1, best_len = 0;
        int cur_start = -1, cur_len = 0;
        char tmp[INET6_ADDRSTRLEN];
        char *tp = tmp;
        int n;

        for (i = 0; i < 8; i++)
            words[i] = ((unsigned int)p[i * 2] << 8) | p[i * 2 + 1];

        /* Find longest run of zero words (RFC 5952: first longest wins) */
        for (i = 0; i < 8; i++) {
            if (words[i] == 0) {
                if (cur_start < 0)
                    cur_start = i;
                cur_len++;
            } else {
                if (cur_len > best_len && cur_len >= 2) {
                    best_start = cur_start;
                    best_len = cur_len;
                }
                cur_start = -1;
                cur_len = 0;
            }
        }
        if (cur_len > best_len && cur_len >= 2) {
            best_start = cur_start;
            best_len = cur_len;
        }

        /* Format with :: compression */
        for (i = 0; i < 8; i++) {
            if (i == best_start) {
                *tp++ = ':';
                *tp++ = ':';
                i += best_len - 1;
                continue;
            }
            if (i > 0 && !(i == best_start + best_len && best_start >= 0))
                *tp++ = ':';
            tp += solcompat_snprintf(tp, sizeof(tmp) - (size_t)(tp - tmp),
                                     "%x", words[i]);
        }
        *tp = '\0';

        n = (int)strlen(tmp);
        if ((socklen_t)(n + 1) > size) {
            errno = ENOSPC;
            return NULL;
        }
        memcpy(dst, tmp, (size_t)(n + 1));
        return dst;
    }
    errno = EAFNOSUPPORT;
    return NULL;
}

/*
 * inet_pton — convert presentation string to binary address
 */
int
inet_pton(int af, const char *src, void *dst)
{
    if (af == AF_INET) {
        struct in_addr addr;
        if (inet_aton(src, &addr) == 0)
            return 0;
        memcpy(dst, &addr, sizeof(addr));
        return 1;
    }
    if (af == AF_INET6) {
        /*
         * Minimal IPv6 address parser: handles full 8-group hex
         * notation (a:b:c:d:e:f:g:h) and the :: abbreviation.
         * Does NOT handle embedded IPv4 (::ffff:1.2.3.4).
         */
        unsigned char buf[16];
        unsigned int groups[8];
        int ngroups = 0, dcolon = -1;
        const char *p = src;
        int i;

        memset(buf, 0, sizeof(buf));
        memset(groups, 0, sizeof(groups));

        /* Handle leading :: */
        if (p[0] == ':' && p[1] == ':') {
            dcolon = 0;
            p += 2;
            if (*p == '\0') {
                /* :: = all zeros */
                memcpy(dst, buf, 16);
                return 1;
            }
        }

        while (*p && ngroups < 8) {
            char *endptr;
            unsigned long val;

            if (!isxdigit((unsigned char)*p))
                return 0;

            val = strtoul(p, &endptr, 16);
            if (val > 0xFFFF)
                return 0;
            groups[ngroups++] = (unsigned int)val;
            p = endptr;

            if (*p == ':') {
                p++;
                if (*p == ':') {
                    if (dcolon >= 0)
                        return 0; /* only one :: allowed */
                    dcolon = ngroups;
                    p++;
                    if (*p == '\0')
                        break;
                }
            } else if (*p != '\0') {
                return 0;
            }
        }

        if (*p != '\0')
            return 0;

        if (dcolon >= 0) {
            /* Expand :: by shifting groups right */
            int tail = ngroups - dcolon;
            int fill = 8 - ngroups;
            if (fill < 0)
                return 0;
            for (i = tail - 1; i >= 0; i--)
                groups[dcolon + fill + i] = groups[dcolon + i];
            for (i = 0; i < fill; i++)
                groups[dcolon + i] = 0;
            ngroups = 8;
        }

        if (ngroups != 8)
            return 0;

        for (i = 0; i < 8; i++) {
            buf[i * 2]     = (unsigned char)(groups[i] >> 8);
            buf[i * 2 + 1] = (unsigned char)(groups[i] & 0xFF);
        }
        memcpy(dst, buf, 16);
        return 1;
    }
    errno = EAFNOSUPPORT;
    return -1;
}

/*
 * getifaddrs — enumerate network interfaces
 * Uses SIOCGIFCONF ioctl which is available on Solaris 7
 */
int
getifaddrs(struct ifaddrs **ifap)
{
    int sock;
    struct ifconf ifc;
    struct ifreq *ifr;
    char buf[4096];
    struct ifaddrs *head = NULL, **tail = &head;
    int i, n;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return -1;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;

    if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
        close(sock);
        return -1;
    }

    n = ifc.ifc_len / sizeof(struct ifreq);
    ifr = ifc.ifc_req;

    for (i = 0; i < n; i++) {
        struct ifaddrs *ifa;
        struct sockaddr_in *addr, *mask;
        struct ifreq flagreq, maskreq;

        ifa = (struct ifaddrs *)calloc(1, sizeof(struct ifaddrs));
        if (!ifa) {
            freeifaddrs(head);
            close(sock);
            errno = ENOMEM;
            return -1;
        }

        ifa->ifa_name = strdup(ifr[i].ifr_name);

        /* Get flags */
        memset(&flagreq, 0, sizeof(flagreq));
        strcpy(flagreq.ifr_name, ifr[i].ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &flagreq) == 0)
            ifa->ifa_flags = flagreq.ifr_flags;

        /* Address */
        addr = (struct sockaddr_in *)calloc(1, sizeof(*addr));
        if (addr) {
            memcpy(addr, &ifr[i].ifr_addr, sizeof(struct sockaddr_in));
            ifa->ifa_addr = (struct sockaddr *)addr;
        }

        /* Netmask */
        memset(&maskreq, 0, sizeof(maskreq));
        strcpy(maskreq.ifr_name, ifr[i].ifr_name);
        if (ioctl(sock, SIOCGIFNETMASK, &maskreq) == 0) {
            mask = (struct sockaddr_in *)calloc(1, sizeof(*mask));
            if (mask) {
                memcpy(mask, &maskreq.ifr_addr, sizeof(struct sockaddr_in));
                ifa->ifa_netmask = (struct sockaddr *)mask;
            }
        }

        ifa->ifa_next = NULL;
        *tail = ifa;
        tail = &ifa->ifa_next;
    }

    close(sock);
    *ifap = head;
    return 0;
}

void
freeifaddrs(struct ifaddrs *ifa)
{
    struct ifaddrs *next;
    while (ifa) {
        next = ifa->ifa_next;
        if (ifa->ifa_name)   free(ifa->ifa_name);
        if (ifa->ifa_addr)   free(ifa->ifa_addr);
        if (ifa->ifa_netmask) free(ifa->ifa_netmask);
        if (ifa->ifa_broadaddr) free(ifa->ifa_broadaddr);
        free(ifa);
        ifa = next;
    }
}

/*
 * if_nametoindex — map interface name to index (RFC 3493)
 *
 * Uses SIOCGIFINDEX ioctl.  On Solaris 7 this ioctl may not exist
 * (it was added in later Solaris versions), so we fall back to
 * scanning SIOCGIFCONF and assigning indices based on position.
 */
#ifndef HAVE_IF_NAMETOINDEX
unsigned int
if_nametoindex(const char *ifname)
{
    int sock;

    if (!ifname)
        return 0;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return 0;

    /*
     * Solaris 7 likely lacks SIOCGIFINDEX, so we always use the
     * SIOCGIFCONF fallback: scan the interface list and assign
     * 1-based indices by position.
     */
    {
        struct ifconf ifc;
        char buf[4096];
        struct ifreq *ifr_p;
        int n, i;

        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;

        if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
            close(sock);
            return 0;
        }

        n = ifc.ifc_len / sizeof(struct ifreq);
        ifr_p = ifc.ifc_req;

        for (i = 0; i < n; i++) {
            if (strcmp(ifr_p[i].ifr_name, ifname) == 0) {
                close(sock);
                return (unsigned int)(i + 1);  /* indices are 1-based */
            }
        }
    }

    close(sock);
    return 0;
}

char *
if_indextoname(unsigned int ifindex, char *ifname)
{
    int sock;
    struct ifconf ifc;
    char buf[4096];
    struct ifreq *ifr_p;
    int n;

    if (ifindex == 0 || !ifname)
        return NULL;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return NULL;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;

    if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
        close(sock);
        return NULL;
    }

    n = ifc.ifc_len / sizeof(struct ifreq);
    ifr_p = ifc.ifc_req;

    /* ifindex is 1-based (matching if_nametoindex fallback) */
    if ((int)ifindex <= n) {
        strncpy(ifname, ifr_p[ifindex - 1].ifr_name, IFNAMSIZ - 1);
        ifname[IFNAMSIZ - 1] = '\0';
        close(sock);
        return ifname;
    }

    close(sock);
    return NULL;
}
#endif /* HAVE_IF_NAMETOINDEX */

/*
 * accept4 — accept a connection with flags (SOCK_CLOEXEC, SOCK_NONBLOCK).
 *
 * Solaris 7 has accept() but not accept4().  We emulate by calling accept()
 * then setting the flags via fcntl().  There is a small window between
 * accept() and fcntl() where the fd lacks the flags — unavoidable without
 * kernel support, but harmless in practice.
 */
int
accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
    int accepted_fd;
    int fcntl_flags;

    accepted_fd = accept(sockfd, addr, addrlen);
    if (accepted_fd < 0)
        return -1;

    if (flags & SOCK_CLOEXEC) {
        if (fcntl(accepted_fd, F_SETFD, FD_CLOEXEC) < 0) {
            close(accepted_fd);
            return -1;
        }
    }

    if (flags & SOCK_NONBLOCK) {
        fcntl_flags = fcntl(accepted_fd, F_GETFL);
        if (fcntl_flags < 0 || fcntl(accepted_fd, F_SETFL, fcntl_flags | O_NONBLOCK) < 0) {
            close(accepted_fd);
            return -1;
        }
    }

    return accepted_fd;
}
