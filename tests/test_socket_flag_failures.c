/* Inject flag-setting failure to check descriptor cleanup and errno. */
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
static int closed_count,fail_fd;
static int native_socket(int domain,int type,int protocol)
{
    (void)domain;(void)protocol;
    if(type!=SOCK_STREAM){errno=EPROTOTYPE;return -1;}
    return 41;
}
static int native_pair(int domain,int type,int protocol,int pair[2])
{
    (void)domain;(void)protocol;
    if(type!=SOCK_STREAM){errno=EPROTOTYPE;return -1;}
    pair[0]=50;pair[1]=51;return 0;
}
static int fail_fcntl(int fd,int command,...)
{
    (void)command;
    if(fd==fail_fd){errno=EMFILE;return -1;}
    return 0;
}
static int noisy_close(int fd)
{
    (void)fd;closed_count++;errno=EIO;return 0;
}
#undef socket
#undef socketpair
#define socket native_socket
#define socketpair native_pair
#define fcntl fail_fcntl
#define close noisy_close
#include "../src/socket_flags.c"
int main(void)
{
    int pair[2]={7,8};
    fail_fd=41;closed_count=0;
    if(solcompat_socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,0)!=-1 ||
       errno!=EMFILE || closed_count!=1)return 1;
    fail_fd=51;closed_count=0;
    if(solcompat_socketpair(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,0,pair)!=-1 ||
       errno!=EMFILE || closed_count!=2 || pair[0]!=7 || pair[1]!=8)return 2;
    if(solcompat_socketpair(AF_UNIX,SOCK_STREAM,0,NULL)!=-1 || errno!=EFAULT)return 3;
    puts("socket flag failure cleanup, unchanged output and errno passed");
    return 0;
}
