#include <solcompat/network.h>
#include <string.h>
#include <stdio.h>
int main(void)
{
    struct sockaddr_in6 v6;
    struct sockaddr_in v4;
    char host[64], service[16];
    memset(&v6, 0, sizeof(v6));
    v6.sin6_family = AF_INET6;
    v6.sin6_scope_id = 7;
    v6.sin6_port = htons(65535);
    if (inet_pton(AF_INET6,"fe80::1",&v6.sin6_addr) != 1) return 1;
    struct sockaddr *addr = (struct sockaddr *)&v6;
    int flags = NI_NUMERICHOST | NI_NUMERICSERV;
    if (getnameinfo(addr,sizeof(v6),host,sizeof(host),service,sizeof(service),flags) ||
        strcmp(host,"fe80::1%7") || strcmp(service,"65535") ||
        getnameinfo(addr,1,host,sizeof(host),NULL,0,flags) != EAI_FAMILY ||
        getnameinfo(addr,sizeof(v6),NULL,0,service,4,flags) != EAI_OVERFLOW ||
        getnameinfo(addr,sizeof(v6),NULL,0,NULL,0,0) != EAI_NONAME ||
        getnameinfo(addr,sizeof(v6),host,sizeof(host),NULL,0,0x40000000) != EAI_BADFLAGS ||
        getnameinfo(addr,sizeof(v6),host,sizeof(host),NULL,0,NI_NAMEREQD) != EAI_NONAME)
        return 2;
    memset(&v4, 0, sizeof v4);
    v4.sin_family = AF_INET;
    v4.sin_port = htons(80);
    if (getnameinfo((struct sockaddr *)&v4,sizeof(v4),NULL,0,service,sizeof(service),0) ||
        !*service) return 3;
    puts("PASS resolver argument checks, scoped numeric host and service sizes");
    return 0;
}
