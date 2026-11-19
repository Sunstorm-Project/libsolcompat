/*
 * network.c — Missing networking APIs for Solaris 7
 *
 * getaddrinfo/freeaddrinfo/gai_strerror/getnameinfo, 
 * inet_ntop/inet_pton, getifaddrs/freeifaddrs
 *
 * Solaris 7 resolves named IPv4 hosts through its native database.
 * Numeric IPv6 addresses, including zones, are handled locally.
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
#include <limits.h>

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

/* Build the socket-type/service combinations independently. Services can
 * have different port numbers for TCP and UDP in the Solaris database. */
struct service_choice {
    int type, protocol;
    unsigned short port;
};

static int service_choices(const char *service, int flags, int type, int protocol,
                           struct service_choice choices[2], int *count)
{
    const int types[] = { SOCK_STREAM, SOCK_DGRAM };
    const int protocols[] = { IPPROTO_TCP, IPPROTO_UDP };
    const char *names[] = { "tcp", "udp" };
    unsigned int port = 0;
    int numeric = !service || (*service && strspn(service, "0123456789") == strlen(service));
    if (type == SOCK_RAW) {
        if (service)
            return EAI_SERVICE;
        choices[0] = (struct service_choice){SOCK_RAW, protocol, 0};
        *count = 1;
        return 0;
    }
    if (type && type != SOCK_STREAM && type != SOCK_DGRAM)
        return EAI_SOCKTYPE;
    if ((protocol && protocol != IPPROTO_TCP && protocol != IPPROTO_UDP) ||
        (type == SOCK_STREAM && protocol == IPPROTO_UDP) ||
        (type == SOCK_DGRAM && protocol == IPPROTO_TCP))
        return EAI_SERVICE;
    if (service && numeric) {
        for (const char *p = service; *p; ++p) {
            port = port * 10 + (*p - '0');
            if (port > 65535)
                return EAI_SERVICE;
        }
    } else if (!numeric && (flags & AI_NUMERICSERV)) {
        return EAI_NONAME;
    }

    *count = 0;
    for (int i = 0; i < 2; ++i) {
        struct servent entry;
        char scratch[4096];
        if ((type && types[i] != type) || (protocol && protocols[i] != protocol))
            continue;
        unsigned short netport = htons((unsigned short)port);
        if (!numeric) {
            if (!getservbyname_r(service, names[i], &entry, scratch, sizeof(scratch)))
                continue;
            netport = entry.s_port;
        }
        choices[(*count)++] = (struct service_choice){types[i], protocols[i], netport};
    }
    return *count ? 0 : EAI_SERVICE;
}

static int numeric_ipv6(const char *node, struct in6_addr *addr, unsigned int *scope)
{
    const char *zone = strchr(node, '%');
    if (!zone)
        return inet_pton(AF_INET6, node, addr) == 1;
    char host[INET6_ADDRSTRLEN];
    size_t len = zone - node;
    if (len >= sizeof(host) || !zone[1])
        return 0;
    memcpy(host, node, len);
    host[len] = 0;
    if (inet_pton(AF_INET6, host, addr) != 1)
        return 0;
    ++zone;
    if (strspn(zone, "0123456789") == strlen(zone)) {
        unsigned int n = 0;
        while (*zone) {
            unsigned int digit = *zone++ - '0';
            if (n > (UINT_MAX-digit)/10)
                return 0;
            n = n*10 + digit;
        }
        *scope = n;
    } else if ((*scope = if_nametoindex(zone)) == 0) {
        return 0;
    }
    return 1;
}

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints, struct addrinfo **res)
{
    int flags = hints ? hints->ai_flags : 0;
    int family = hints ? hints->ai_family : AF_UNSPEC;
    struct service_choice choices[2];
    int count, error, herr = 0;
    struct addrinfo *head = NULL, **tail = &head;
    struct hostent entry;
    char scratch[8192];
    struct in_addr v4;
    struct in6_addr v6;
    char *local[] = { NULL, NULL }, **addresses = local;
    const char *canonical = node;
    unsigned int scope = 0;

    if (!res || (!node && !service))
        return EAI_NONAME;
    *res = NULL;
    if (family != AF_INET && family != AF_INET6 && family != AF_UNSPEC)
        return EAI_FAMILY;
    if (flags & ~(AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST | AI_NUMERICSERV | AI_ADDRCONFIG))
        return EAI_BADFLAGS;
    error = service_choices(service, flags, hints ? hints->ai_socktype : 0,
                            hints ? hints->ai_protocol : 0, choices, &count);
    if (error)
        return error;

    if (node && family != AF_INET && numeric_ipv6(node, &v6, &scope)) {
        family = AF_INET6;
        local[0] = (char *)&v6;
    } else if (!node && family == AF_INET6) {
        v6 = flags & AI_PASSIVE ? in6addr_any : in6addr_loopback;
        local[0] = (char *)&v6;
    } else if (family == AF_INET6) {
        /* Solaris 7's legacy host database returns IPv4 records. */
        return EAI_NONAME;
    } else {
        family = AF_INET;
        local[0] = (char *)&v4;
        if (!node) {
            v4.s_addr = flags & AI_PASSIVE ? INADDR_ANY : htonl(INADDR_LOOPBACK);
        } else if (!inet_aton(node, &v4)) {
            if (flags & AI_NUMERICHOST)
                return EAI_NONAME;
            if (!gethostbyname_r(node, &entry, scratch, sizeof(scratch), &herr))
                return herr == TRY_AGAIN ? EAI_AGAIN : herr == NO_RECOVERY ? EAI_FAIL : EAI_NONAME;
            if (entry.h_addrtype != AF_INET || entry.h_length != sizeof(v4))
                return EAI_FAMILY;
            addresses = entry.h_addr_list;
            canonical = entry.h_name ? entry.h_name : node;
        }
    }

    if (flags & AI_ADDRCONFIG) {
        struct ifaddrs *ifs, *it;
        int configured = 0;
        if (getifaddrs(&ifs) < 0)
            return EAI_SYSTEM;
        for (it = ifs; it; it = it->ifa_next)
            if (it->ifa_addr && it->ifa_addr->sa_family == family &&
                !(it->ifa_flags & IFF_LOOPBACK)) {
                configured = 1;
                break;
            }
        freeifaddrs(ifs);
        if (!configured)
            return EAI_NONAME;
    }

    for (int t = 0; t < count; ++t) {
        for (char **addr = addresses; addr && *addr; ++addr) {
            size_t len = family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            struct addrinfo *ai = calloc(1, sizeof(*ai) + len);
            if (!ai)
                goto nomem;
            *tail = ai;
            tail = &ai->ai_next;
            ai->ai_family = family;
            ai->ai_socktype = choices[t].type;
            ai->ai_protocol = choices[t].protocol;
            ai->ai_addrlen = len;
            ai->ai_addr = (struct sockaddr *)(ai + 1);
            if (family == AF_INET) {
                struct sockaddr_in *sin = (struct sockaddr_in *)ai->ai_addr;
                sin->sin_family = family;
                sin->sin_port = choices[t].port;
                memcpy(&sin->sin_addr, *addr, sizeof(v4));
            } else {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ai->ai_addr;
                sin6->sin6_family = family;
                sin6->sin6_port = choices[t].port;
                sin6->sin6_scope_id = scope;
                memcpy(&sin6->sin6_addr, *addr, sizeof(v6));
            }
            if (ai == head && (flags & AI_CANONNAME) && canonical &&
                !(ai->ai_canonname = strdup(canonical)))
                goto nomem;
        }
    }
    if (!head)
        return EAI_NONAME;
    *res = head;
    return 0;
nomem:
    freeaddrinfo(head);
    return EAI_MEMORY;
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
    case EAI_OVERFLOW: return "Output buffer too small";
    default:           return "Unknown error";
    }
}

int
getnameinfo(const struct sockaddr *sa, socklen_t salen,
            char *host, socklen_t hostlen,
            char *serv, socklen_t servlen, int flags)
{
    unsigned short port;
    const void *address;
    const char *proto = flags & NI_DGRAM ? "udp" : "tcp";
    unsigned int scope = 0;

    if (!sa || salen < sizeof(sa->sa_family))
        return EAI_FAMILY;
    if (flags & ~(NI_NUMERICHOST | NI_NUMERICSERV | NI_NAMEREQD | NI_NOFQDN | NI_DGRAM))
        return EAI_BADFLAGS;
    if ((!host || !hostlen) && (!serv || !servlen))
        return EAI_NONAME;
    if (sa->sa_family == AF_INET && salen >= sizeof(struct sockaddr_in)) {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
        port = sin->sin_port;
        address = &sin->sin_addr;
    } else if (sa->sa_family == AF_INET6 && salen >= sizeof(struct sockaddr_in6)) {
        const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
        port = sin6->sin6_port;
        address = &sin6->sin6_addr;
        scope = sin6->sin6_scope_id;
    } else {
        return EAI_FAMILY;
    }

    if (host && hostlen) {
        struct hostent entry, *found = NULL;
        char scratch[8192], numeric[INET6_ADDRSTRLEN + 12];
        const char *name;
        int resolver_error = 0;
        if (!(flags & NI_NUMERICHOST) && sa->sa_family == AF_INET)
            found = gethostbyaddr_r((const char *)address, sizeof(struct in_addr),
                                    AF_INET, &entry, scratch, sizeof(scratch), &resolver_error);
        if (found && found->h_name) {
            name = found->h_name;
        } else {
            if (!(flags & NI_NUMERICHOST) && (flags & NI_NAMEREQD))
                return resolver_error == TRY_AGAIN ? EAI_AGAIN : EAI_NONAME;
            if (!inet_ntop(sa->sa_family, address, numeric, sizeof(numeric)))
                return EAI_SYSTEM;
            if (scope) {
                size_t len = strlen(numeric);
                solcompat_snprintf(numeric + len, sizeof(numeric) - len, "%%%u", scope);
            }
            name = numeric;
        }
        if (strlen(name) >= hostlen)
            return EAI_OVERFLOW;
        strcpy(host, name);
    }
    if (serv && servlen) {
        struct servent entry, *found = NULL;
        char scratch[4096], numeric[6];
        const char *name;
        if (!(flags & NI_NUMERICSERV))
            found = getservbyport_r(port, proto, &entry, scratch, sizeof(scratch));
        if (found && found->s_name) {
            name = found->s_name;
        } else {
            solcompat_snprintf(numeric, sizeof(numeric), "%u", (unsigned int)ntohs(port));
            name = numeric;
        }
        if (strlen(name) >= servlen)
            return EAI_OVERFLOW;
        strcpy(serv, name);
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

        if (best_start == 0 && best_len == 5 && words[5] == 0xffff) {
            n = solcompat_snprintf(tmp, sizeof(tmp), "::ffff:%u.%u.%u.%u",
                                   p[12], p[13], p[14], p[15]);
            if ((socklen_t)n >= size) {
                errno = ENOSPC;
                return NULL;
            }
            memcpy(dst, tmp, n + 1);
            return dst;
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

/* RFC 3493 dotted decimal. inet_aton also accepts legacy abbreviated,
 * octal, and hexadecimal forms that inet_pton must reject. */
static int decimal_address(const char *p, unsigned char out[4])
{
    for (int i = 0; i < 4; ++i) {
        unsigned int n = 0;
        if (*p < '0' || *p > '9' || (*p == '0' && p[1] >= '0' && p[1] <= '9'))
            return 0;
        do {
            n = n * 10 + (*p++ - '0');
            if (n > 255)
                return 0;
        } while (*p >= '0' && *p <= '9');
        out[i] = n;
        if (i < 3 && *p++ != '.')
            return 0;
    }
    return *p == 0;
}

int inet_pton(int af, const char *src, void *dst)
{
    unsigned char buf[16];
    int used = 0, gap = -1;
    const char *p = src;
    if (af == AF_INET) {
        if (!decimal_address(p, buf))
            return 0;
        memcpy(dst, buf, 4);
        return 1;
    }
    if (af != AF_INET6) {
        errno = EAFNOSUPPORT;
        return -1;
    }
    if (*p == ':') {
        if (p[1] != ':')
            return 0;
        gap = 0;
        p += 2;
    }
    while (*p) {
        const char *start = p;
        unsigned int value = 0;
        if (used >= 16)
            return 0;
        while (isxdigit((unsigned char)*p)) {
            unsigned char c = *p++;
            if (p - start > 4)
                return 0;
            value = value * 16 + (c <= '9' ? c - '0' : (c | 32) - 'a' + 10);
        }
        if (*p == '.') {
            if (used > 12 || !decimal_address(start, buf + used))
                return 0;
            used += 4;
            break;
        }
        if (p == start)
            return 0;
        buf[used++] = value >> 8;
        buf[used++] = value;
        if (!*p)
            break;
        if (*p++ != ':' || !*p)
            return 0;
        if (*p == ':') {
            if (gap >= 0)
                return 0;
            gap = used;
            ++p;
        }
    }
    if (gap >= 0 && used < 16) {
        int missing = 16 - used;
        memmove(buf + gap + missing, buf + gap, used - gap);
        memset(buf + gap, 0, missing);
    } else if (used != 16 || gap >= 0) {
        return 0;
    }
    memcpy(dst, buf, sizeof(buf));
    return 1;
}

/* Leave room for an interface added between the count and list ioctls.
 * If the returned buffer is full, grow and enumerate again. */
static struct ifreq *interface_list(int sock, int *count)
{
    int capacity;
    struct ifconf conf;
    struct ifreq *buf;
    if (ioctl(sock, SIOCGIFNUM, &capacity) < 0)
        return NULL;
    if (capacity < 8)
        capacity = 8;
    for (;;) {
        if (capacity > INT_MAX / (2 * (int)sizeof(*buf))) {
            errno = ENOBUFS;
            return NULL;
        }
        capacity *= 2;
        conf.ifc_len = capacity * sizeof(*buf);
        buf = malloc(conf.ifc_len);
        if (!buf)
            return NULL;
        conf.ifc_req = buf;
        if (ioctl(sock, SIOCGIFCONF, &conf) < 0) {
            free(buf);
            return NULL;
        }
        *count = conf.ifc_len / sizeof(*buf);
        if (*count < capacity)
            return buf;
        free(buf);
    }
}

/*
 * getifaddrs — enumerate network interfaces
 * Uses SIOCGIFCONF ioctl which is available on Solaris 7
 */
int
getifaddrs(struct ifaddrs **ifap)
{
    int sock;
    struct ifreq *ifr;
    struct ifaddrs *head = NULL, **tail = &head;
    int i, n;

    if (!ifap) {
        errno = EINVAL;
        return -1;
    }
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return -1;
    ifr = interface_list(sock, &n);
    if (!ifr) {
        close(sock);
        return -1;
    }

    for (i = 0; i < n; i++) {
        struct ifaddrs *ifa;
        struct sockaddr_in *addr, *mask;
        struct ifreq flagreq, maskreq;

        ifa = (struct ifaddrs *)calloc(1, sizeof(struct ifaddrs));
        if (!ifa)
            goto nomem;
        *tail = ifa;
        tail = &ifa->ifa_next;
        ifa->ifa_name = strdup(ifr[i].ifr_name);
        if (!ifa->ifa_name)
            goto nomem;

        /* Get flags */
        memset(&flagreq, 0, sizeof(flagreq));
        strcpy(flagreq.ifr_name, ifr[i].ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &flagreq) == 0)
            ifa->ifa_flags = flagreq.ifr_flags;

        /* Address */
        addr = (struct sockaddr_in *)calloc(1, sizeof(*addr));
        if (!addr)
            goto nomem;
        memcpy(addr, &ifr[i].ifr_addr, sizeof(*addr));
        ifa->ifa_addr = (struct sockaddr *)addr;

        /* Netmask */
        memset(&maskreq, 0, sizeof(maskreq));
        strcpy(maskreq.ifr_name, ifr[i].ifr_name);
        if (ioctl(sock, SIOCGIFNETMASK, &maskreq) == 0) {
            mask = (struct sockaddr_in *)calloc(1, sizeof(*mask));
            if (!mask)
                goto nomem;
            memcpy(mask, &maskreq.ifr_addr, sizeof(*mask));
            ifa->ifa_netmask = (struct sockaddr *)mask;
        }
    }

    free(ifr);
    close(sock);
    *ifap = head;
    return 0;
nomem:
    freeifaddrs(head);
    free(ifr);
    close(sock);
    errno = ENOMEM;
    return -1;
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

/* SIOCGIFINDEX returns the kernel index used by sockets. */
static unsigned int interface_index(int sock, const char *name)
{
    struct ifreq req;
    if (!name || !*name || strlen(name) >= sizeof(req.ifr_name)) {
        errno = ENXIO;
        return 0;
    }
    memset(&req, 0, sizeof(req));
    strcpy(req.ifr_name, name);
    if (ioctl(sock, SIOCGIFINDEX, &req) < 0)
        return 0;
    return req.ifr_index;
}

unsigned int if_nametoindex(const char *name)
{
    struct if_nameindex *list;
    unsigned int index = 0;
    size_t i;
    if (!name || !*name || strlen(name) >= IFNAMSIZ) {
        errno = ENXIO;
        return 0;
    }
    list = if_nameindex();
    if (!list)
        return 0;
    for (i = 0; list[i].if_name; ++i)
        if (strcmp(list[i].if_name, name) == 0) {
            index = list[i].if_index;
            break;
        }
    if_freenameindex(list);
    if (!index)
        errno = ENXIO;
    return index;
}

void if_freenameindex(struct if_nameindex *list)
{
    size_t i;
    if (list) {
        for (i = 0; list[i].if_name; ++i)
            free(list[i].if_name);
        free(list);
    }
}

struct if_nameindex *if_nameindex(void)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct if_nameindex *list = NULL;
    struct ifreq *rows;
    int count, i, used = 0, use_positions = 0;
    if (sock < 0)
        return NULL;
    rows = interface_list(sock, &count);
    if (!rows)
        goto done;
    list = calloc(count + 1, sizeof(*list));
    if (!list)
        goto done;
    for (i = 0; i < count; ++i) {
        char *name = rows[i].ifr_name;
        unsigned int index;
        int j;
        for (j = 0; j < used && strcmp(list[j].if_name, name); ++j)
            ;
        if (j < used)
            continue;
        index = interface_index(sock, name);
        if (!index) {
            /* Solaris 7 exposes the ioctl in its headers, but some kernels
             * return ENOENT even for lo0. Preserve the existing 1-based
             * convention consistently across all three lookup functions. */
            if (errno != ENOENT && errno != ENOTTY && errno != EINVAL &&
                errno != ENOSYS && errno != EOPNOTSUPP)
                goto fail;
            use_positions = 1;
        } else {
            for (j = 0; j < used && list[j].if_index != index; ++j)
                ;
            if (j < used)
                continue;
        }
        list[used].if_name = strdup(name);
        if (!list[used].if_name)
            goto fail;
        list[used++].if_index = index;
    }
    if (use_positions)
        for (i = 0; i < used; ++i)
            list[i].if_index = i + 1;
    goto done;
fail:
    if_freenameindex(list);
    list = NULL;
done:
    free(rows);
    close(sock);
    return list;
}

char *if_indextoname(unsigned int index, char *name)
{
    struct if_nameindex *list;
    size_t i;
    if (!name || index == 0) {
        errno = ENXIO;
        return NULL;
    }
    list = if_nameindex();
    if (!list)
        return NULL;
    for (i = 0; list[i].if_name; ++i) {
        if (list[i].if_index == index) {
            strcpy(name, list[i].if_name);
            if_freenameindex(list);
            return name;
        }
    }
    if_freenameindex(list);
    errno = ENXIO;
    return NULL;
}

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
