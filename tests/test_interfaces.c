#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <solcompat/network.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
int main(void)
{
    struct if_nameindex *all = if_nameindex();
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    int i;
    char name[IF_NAMESIZE];
    if (fd < 0 || !all || !all[0].if_name) {
        perror("interface list");
        return 1;
    }
    for (i = 0; all[i].if_name; ++i) {
        struct ifreq req;
        memset(&req, 0, sizeof(req));
        strcpy(req.ifr_name, all[i].if_name);
        int rc = ioctl(fd, SIOCGIFINDEX, &req);
        if ((rc == 0 && (unsigned int)req.ifr_index != all[i].if_index) ||
            !all[i].if_index ||
            if_nametoindex(all[i].if_name) != all[i].if_index ||
            !if_indextoname(all[i].if_index, name) || strcmp(name, all[i].if_name))
            return 2;
        printf("%s %u\n", name, all[i].if_index);
    }
    if (all[i].if_index || if_indextoname(~0U, name) || errno != ENXIO)
        return 3;
    struct ifaddrs *addresses, *it;
    if (getifaddrs(&addresses) < 0 || !addresses)
        return 4;
    for (it = addresses; it; it = it->ifa_next)
        if (!it->ifa_addr || !it->ifa_name || !if_nametoindex(it->ifa_name))
            return 5;
    freeifaddrs(addresses);
    if_freenameindex(all);
    close(fd);
    return 0;
}
