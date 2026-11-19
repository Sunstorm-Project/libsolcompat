#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
extern char *solcompat_getcwd(char *, size_t);
int main(void) {
    char buffer[4096], shortbuf[1], *p;
    if (!getcwd(buffer,sizeof buffer)) return 1;
    p=solcompat_getcwd(NULL,0);
    if (!p || strcmp(p,buffer)) return 2;
    free(p);
    p=getcwd(NULL,0);
    if (!p || strcmp(p,buffer)) return 3;
    free(p);
    p=getcwd(NULL,sizeof buffer);
    if (!p || strcmp(p,buffer)) return 4;
    free(p);
    errno=0;
    if (getcwd(shortbuf,sizeof shortbuf) || errno!=ERANGE) return 5;
    errno=0;
    if (getcwd(buffer,0) || errno!=EINVAL) return 6;
    {
        char top[64];
        const char *part = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefgh";
        int level;
        snprintf(top, sizeof top, "getcwd-test-%ld", (long)getpid());
        if (mkdir(top,0700) || chdir(top)) return 7;
        for (level=0; level<4; ++level)
            if (mkdir(part,0700) || chdir(part)) return 8;
        if (!getcwd(buffer,sizeof buffer) || strlen(buffer)<=128) return 9;
        p=getcwd(NULL,0);
        if (!p || strcmp(p,buffer)) return 10;
        free(p);
        for (level=0; level<4; ++level)
            if (chdir("..") || rmdir(part)) return 11;
        if (chdir("..") || rmdir(top)) return 12;
    }
    puts("allocating, growth, and native getcwd contracts passed");
    return 0;
}
