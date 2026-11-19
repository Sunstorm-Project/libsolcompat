#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <solcompat/filesystem.h>
int main(void)
{
    char path[] = "/tmp/fdopendir-XXXXXX", moved[96], file[128];
    DIR *d;
    struct dirent *e;
    int fd, regular, seen = 0;
    if (!mkdtemp(path)) return 1;
    snprintf(moved,sizeof moved,"%s-moved",path);
    snprintf(file,sizeof file,"%s/entry",path);
    regular = open(file,O_RDWR|O_CREAT|O_EXCL,0600);
    if (regular < 0 || fdopendir(regular) || errno != ENOTDIR ||
        fcntl(regular,F_GETFD) < 0 || close(regular)) return 2;
    fd = open(path,O_RDONLY);
    if (fd < 0 || rename(path,moved) || !(d = fdopendir(fd)) || dirfd(d) != fd) return 3;
    while ((e = readdir(d)) != NULL)
        if (!strcmp(e->d_name,"entry")) seen = 1;
    if (!seen || closedir(d) || fcntl(fd,F_GETFD) != -1 || errno != EBADF) return 4;
    snprintf(file,sizeof file,"%s/entry",moved);
    if (unlink(file) || rmdir(moved)) return 5;
    puts("PASS fdopendir ownership, closedir, rename and error preservation");
    return 0;
}
