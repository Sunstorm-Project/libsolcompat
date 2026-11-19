#include <solcompat/network.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
int main(void)
{
    static const char *bad4[] = {"127.1","0177.0.0.1","0x7f.0.0.1","1.2.3.256","1.2.3."," 1.2.3.4"};
    static const char *bad6[] = {"1:2:3:4:5:6:7:8:", "1:2:3:4:5:6:7:8::",
        "::1::", "00000::1", "0x1::", "1:2:3:4:5:6:7:192.0.2.1"};
    unsigned char addr[16], original[16];
    char text[46];
    unsigned int i;
    memset(original, 42, sizeof original);
    memcpy(addr, original, sizeof addr);
    for (i = 0; i < sizeof bad4 / sizeof *bad4; ++i)
        if (inet_pton(AF_INET, bad4[i], addr) || memcmp(addr, original, 16)) return 1;
    for (i = 0; i < sizeof bad6 / sizeof *bad6; ++i)
        if (inet_pton(AF_INET6, bad6[i], addr) || memcmp(addr, original, 16)) return 2;
    if (inet_pton(AF_INET6, "::ffff:192.0.2.1", addr) != 1 ||
        addr[10] != 255 || addr[12] != 192 || addr[15] != 1 ||
        !inet_ntop(AF_INET6, addr, text, sizeof text) ||
        strcmp(text, "::ffff:192.0.2.1") ||
        inet_ntop(AF_INET6, addr, text, 5) || errno != ENOSPC) return 3;
    puts("PASS strict dotted decimal, IPv6 compression and mapped-address round trip");
    return 0;
}
