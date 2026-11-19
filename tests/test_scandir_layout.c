#include <solcompat/filesystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
static int selected(const struct dirent *entry)
{
    errno = EDOM; /* A callback may change errno without ending enumeration. */
    return entry->d_name[0] != '.';
}
int main(void)
{
    const char *names[] = {"alpha-with-a-long-name", "zeta-with-a-long-name"};
    char dir[] = "/tmp/scandir-layout-XXXXXX", path[128];
    struct dirent **list;
    if (!mkdtemp(dir)) return 1;
    for (int i = 1; i >= 0; --i) {
        snprintf(path,sizeof path,"%s/%s",dir,names[i]);
        int fd = open(path,O_WRONLY|O_CREAT|O_EXCL,0600);
        if (fd < 0 || close(fd)) return 2;
    }
    if (scandir(dir,&list,selected,alphasort) != 2) return 3;
    for (int i = 0; i < 2; ++i) {
        if (!list[i]->d_ino || strcmp(names[i],list[i]->d_name) ||
            list[i]->d_reclen < offsetof(struct dirent,d_name)+strlen(names[i])+1) return 4;
        free(list[i]);
        snprintf(path,sizeof path,"%s/%s",dir,names[i]);
        if (unlink(path)) return 5;
    }
    free(list);
    if (rmdir(dir)) return 6;
    puts("PASS scandir callback and result layouts, full names and sorting");
    return 0;
}
