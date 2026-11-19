#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0x80000
#endif
#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK 0x800
#endif
extern int solcompat_socket(int,int,int);
extern int solcompat_socketpair(int,int,int,int[2]);
static int has_flags(int fd,int flags)
{
    int status=fcntl(fd,F_GETFL), descriptor=fcntl(fd,F_GETFD);
    return status>=0 && descriptor>=0 &&
        !!(status&O_NONBLOCK)==!!(flags&SOCK_NONBLOCK) &&
        !!(descriptor&FD_CLOEXEC)==!!(flags&SOCK_CLOEXEC);
}
int main(void)
{
    const int variants[]={0,SOCK_CLOEXEC,SOCK_NONBLOCK,SOCK_CLOEXEC|SOCK_NONBLOCK};
    unsigned int i;
    for(i=0;i<sizeof variants/sizeof variants[0];i++) {
        int fd=solcompat_socket(AF_INET,SOCK_STREAM|variants[i],0),pair[2];
        char ch;
        if(fd<0 || !has_flags(fd,variants[i]))return 1;
        close(fd);
        if(solcompat_socketpair(AF_UNIX,SOCK_STREAM|variants[i],0,pair)<0)return 2;
        if(!has_flags(pair[0],variants[i]) || !has_flags(pair[1],variants[i]))return 3;
        if(variants[i]&SOCK_NONBLOCK) {
            errno=0;
            if(read(pair[0],&ch,1)!=-1 || (errno!=EAGAIN && errno!=EWOULDBLOCK))return 4;
        }
        if(write(pair[0],"x",1)!=1 || read(pair[1],&ch,1)!=1 || ch!='x')return 5;
        close(pair[0]);close(pair[1]);
    }
    errno=0;
    if(solcompat_socket(-1,SOCK_STREAM|SOCK_CLOEXEC,0)!=-1 || !errno)return 6;
    puts("socket and socketpair creation flags, nonblocking I/O and native errors passed");
    return 0;
}
