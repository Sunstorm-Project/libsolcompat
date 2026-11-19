#include <solcompat/network.h>
#include <stdio.h>
#include <net/if.h>
#include <string.h>
int main(void)
{
    struct addrinfo h, *list, *a;
    char host[64], service[8];
    memset(&h, 0, sizeof h);
    h.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_CANONNAME;
    if (getaddrinfo("fe80::1%7", "12345", &h, &list)) return 1;
    unsigned int mask = 0;
    for (a = list; a; a = a->ai_next) {
        if (a->ai_family != AF_INET6 ||
            getnameinfo(a->ai_addr, a->ai_addrlen, host, sizeof host,
                        service, sizeof service, NI_NUMERICHOST | NI_NUMERICSERV) ||
            strcmp(host, "fe80::1%7") || strcmp(service, "12345"))
            return 2;
        mask |= a->ai_protocol == IPPROTO_TCP ? 1 : a->ai_protocol == IPPROTO_UDP ? 2 : 0;
    }
    if (mask != 3 || !list->ai_canonname || strcmp(list->ai_canonname,host)) return 3;
    freeaddrinfo(list);
    h.ai_family = AF_INET6;
    h.ai_flags = AI_PASSIVE;
    h.ai_protocol = IPPROTO_UDP;
    if (getaddrinfo(NULL, "80", &h, &list) || list->ai_next ||
        list->ai_socktype != SOCK_DGRAM) return 4;
    freeaddrinfo(list);
    h.ai_family = AF_INET;
    h.ai_socktype = SOCK_STREAM;
    if (getaddrinfo("127.0.0.1","80",&h,&list) != EAI_SERVICE) return 5;
    h.ai_protocol = 0;
    if (getaddrinfo("127.0.0.1","65536",&h,&list) != EAI_SERVICE) return 6;
    h.ai_socktype = SOCK_RAW;
    h.ai_protocol = IPPROTO_ICMP;
    if (getaddrinfo("127.0.0.1",NULL,&h,&list) || list->ai_next ||
        list->ai_socktype != SOCK_RAW || list->ai_protocol != IPPROTO_ICMP) return 7;
    freeaddrinfo(list);
    h.ai_flags = AI_ADDRCONFIG | AI_NUMERICHOST;
    h.ai_socktype = SOCK_STREAM;
    h.ai_protocol = 0;
    struct ifaddrs *ifs, *it;
    int configured = 0;
    if (getifaddrs(&ifs)) return 8;
    for (it = ifs; it; it = it->ifa_next)
        if (it->ifa_addr && it->ifa_addr->sa_family == AF_INET &&
            !(it->ifa_flags & IFF_LOOPBACK)) configured = 1;
    freeifaddrs(ifs);
    if (getaddrinfo("127.0.0.1","80",&h,&list) !=
        (configured ? 0 : EAI_NONAME)) return 9;
    freeaddrinfo(list);
    puts("PASS numeric and passive IPv6, canonical names, services and protocol hints");
    return 0;
}
