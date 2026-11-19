#include <solcompat/at_funcs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
int main(void)
{
    char dir[]="/tmp/relative-nodes-XXXXXX", longpath[1200];
    if (!mkdtemp(dir)) return 1;
    int fd=open(dir,O_RDONLY);
    struct stat st;
    if (fd<0 || mkfifoat(fd,"one",0600) || fstatat(fd,"one",&st,0) || !S_ISFIFO(st.st_mode) ||
        mknodat(fd,"two",S_IFIFO|0600,0) || fstatat(fd,"two",&st,0) || !S_ISFIFO(st.st_mode))
        return 2;
    memset(longpath,'x',sizeof(longpath)-1);longpath[sizeof(longpath)-1]=0;
    if (renameat(fd,"one",fd,longpath)!=-1 || errno!=ENAMETOOLONG ||
        linkat(fd,"one",fd,"unexpected",0x400000)!=-1 || errno!=EINVAL ||
        renameat(fd,"one",fd,"three") || unlinkat(fd,"three",0) || unlinkat(fd,"two",0))
        return 3;
    close(fd);
    if (rmdir(dir)) return 4;
    puts("PASS directory-relative FIFO/node creation, rename bounds and link flags");
    return 0;
}
