#include <solcompat/stdio_ext.h>
#include <solcompat/string_ext.h>
#include <solcompat/pty.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
static int send_fmt(int fd, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vdprintf(fd, fmt, ap);
    va_end(ap);
    return n;
}
int main(void)
{
    int p[2], master, slave;
    char buf[12];
    if (ffsl(0) || ffsl(0x18) != 4 || ffsll(0) ||
        ffsll(1LL<<42) != 43 || ffsll(LLONG_MIN) != sizeof(long long)*CHAR_BIT)
        return 1;
    if (pipe(p) || send_fmt(p[1],"%s:%02d","sst",7) != 6 ||
        read(p[0],buf,sizeof buf) != 6 || memcmp(buf,"sst:07",6) ||
        send_fmt(-1,"%d",42) != -1 || errno != EBADF)
        return 2;
    close(p[0]); close(p[1]);
    if (openpty(&master,&slave,NULL,NULL,NULL)) return 3;
    struct winsize set = {0}, got = {0};
    set.ws_row=41; set.ws_col=107;
    if (tcsetwinsize(slave,&set) || tcgetwinsize(slave,&got) ||
        got.ws_row!=41 || got.ws_col!=107) return 4;
    close(slave); close(master);
    puts("PASS descriptor formatting, wide bit scans and terminal window dimensions");
    return 0;
}
