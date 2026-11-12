#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "solcompat/process.h"
extern char **environ;
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s errno=%d\n",__LINE__,#x,errno); return 1; } } while(0)
static int wait_ok(pid_t pid) { int st; return waitpid(pid,&st,0)==pid && WIFEXITED(st) && WEXITSTATUS(st)==0; }
int main(int argc, char **argv) {
    posix_spawn_file_actions_t a;
    posix_spawnattr_t attr;
    pid_t pid = -123;
    int p[2], n, fd, highest;
    char *oldpath, *dir, *slash;
    char *child_env[]={"PATH=/no/such/child-path", "SST_SPAWN_MARK=child", NULL};
    char *envargs[]={argv[0], "env", NULL};
    char b[32], path[128], copied[128];
    char *args[] = {argv[0], "child", NULL};
    char *attrargs[] = {argv[0], "attrs", NULL};
    char *fdargs[] = {argv[0], "fd", NULL};
    sigset_t mask;
    struct sigaction sa;
    if (argc > 1 && !strcmp(argv[1],"env"))
        return getenv("SST_SPAWN_MARK") && !strcmp(getenv("SST_SPAWN_MARK"),"child") &&
            getenv("PATH") && !strcmp(getenv("PATH"),"/no/such/child-path") ? 0:5;
    if (argc > 1 && !strcmp(argv[1],"attrs")) {
        if (getpgrp()!=getpid() || sigprocmask(SIG_BLOCK,NULL,&mask)<0 ||
            sigismember(&mask,SIGUSR1)!=1 || sigaction(SIGUSR2,NULL,&sa)<0 ||
            sa.sa_handler!=SIG_DFL) return 3;
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1],"fd")) {
        fd=fcntl(9,F_GETFD);
        return fd>=0 && !(fd & FD_CLOEXEC) ? 0 : 4;
    }
    if (argc > 1 && !strcmp(argv[1],"child")) {
        n=read(0,b,sizeof(b));
        return n==11 && !memcmp(b,"spawn bytes",11) && write(1,b,n)==n ? 0:2;
    }
    snprintf(path,sizeof(path),"/tmp/solcompat-spawn-%ld",(long)getpid());
    CHECK(pipe(p)==0);
    CHECK(write(p[1],"spawn bytes",11)==11); CHECK(close(p[1])==0);
    CHECK(posix_spawn_file_actions_init(&a)==0);
    CHECK(posix_spawn_file_actions_adddup2(&a,p[0],0)==0);
    CHECK(posix_spawn_file_actions_addclose(&a,p[0])==0);
    strcpy(copied,path);
    CHECK(posix_spawn_file_actions_addopen(&a,1,copied,O_WRONLY|O_CREAT|O_TRUNC,0600)==0);
    memset(copied,'x',strlen(copied)); /* addopen owns its path copy */
    CHECK(posix_spawn(&pid,argv[0],&a,NULL,args,environ)==0);
    CHECK(wait_ok(pid)); CHECK(close(p[0])==0);
    CHECK(posix_spawn_file_actions_destroy(&a)==0);
    fd=open(path,O_RDONLY); CHECK(fd>=0); n=read(fd,b,sizeof(b)); close(fd); unlink(path);
    CHECK(n==11 && !memcmp(b,"spawn bytes",11));
    CHECK(posix_spawn_file_actions_init(&a)==0);
    CHECK(posix_spawn_file_actions_adddup2(&a,-1,0)==EBADF);
    CHECK(posix_spawn_file_actions_addclose(&a,-1)==EBADF);
    CHECK(posix_spawn_file_actions_addopen(&a,-1,path,0,0)==EBADF);
    close(20);
    CHECK(posix_spawn_file_actions_adddup2(&a,20,1)==0);
    pid=-123;
    CHECK(posix_spawn(&pid,argv[0],&a,NULL,args,environ)==EBADF && pid==-123);
    CHECK(posix_spawn_file_actions_destroy(&a)==0);
    pid=-123;
    CHECK(posix_spawn(&pid,"/no/such/program",NULL,NULL,args,environ)==ENOENT && pid==-123);
    CHECK(posix_spawnattr_init(&attr)==0);
    CHECK(posix_spawnattr_setflags(&attr,POSIX_SPAWN_SETSCHEDULER)==0);
    CHECK(posix_spawn(&pid,argv[0],NULL,&attr,args,environ)==ENOTSUP);
    CHECK(posix_spawnattr_destroy(&attr)==0);
    CHECK(posix_spawnattr_init(&attr)==0);
    sigemptyset(&mask); sigaddset(&mask,SIGUSR1);
    CHECK(posix_spawnattr_setsigmask(&attr,&mask)==0);
    sigfillset(&mask);
    CHECK(posix_spawnattr_setsigdefault(&attr,&mask)==0);
    CHECK(posix_spawnattr_setpgroup(&attr,0)==0);
    CHECK(posix_spawnattr_setflags(&attr,POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETPGROUP)==0);
    CHECK(posix_spawnp(&pid,argv[0],NULL,&attr,attrargs,environ)==0);
    CHECK(wait_ok(pid));
    CHECK(posix_spawnattr_destroy(&attr)==0);
    fd=open("/dev/null",O_RDONLY); CHECK(fd>=0);
    CHECK(dup2(fd,9)==9); if (fd!=9) close(fd);
    CHECK(fcntl(9,F_SETFD,FD_CLOEXEC)==0);
    CHECK(posix_spawn_file_actions_init(&a)==0);
    CHECK(posix_spawn_file_actions_adddup2(&a,9,9)==0);
    CHECK(posix_spawn(&pid,argv[0],&a,NULL,fdargs,environ)==0);
    CHECK(wait_ok(pid)); CHECK(close(9)==0);
    CHECK(posix_spawn_file_actions_destroy(&a)==0);
    /* The hidden error channel must fit below a highest-valid action FD. */
    highest=(int)sysconf(_SC_OPEN_MAX)-1; CHECK(highest>20);
    fd=open("/dev/null",O_RDONLY); CHECK(fd>=0);
    CHECK(dup2(fd,highest)==highest); if (fd!=highest) close(fd);
    CHECK(posix_spawn_file_actions_init(&a)==0);
    CHECK(posix_spawn_file_actions_adddup2(&a,highest,9)==0);
    CHECK(posix_spawn_file_actions_addclose(&a,highest)==0);
    CHECK(posix_spawn(&pid,argv[0],&a,NULL,fdargs,environ)==0);
    CHECK(wait_ok(pid)); CHECK(close(highest)==0);
    CHECK(posix_spawn_file_actions_destroy(&a)==0);
    CHECK(posix_spawn_file_actions_init(&a)==0);
    CHECK(posix_spawn_file_actions_adddup2(&a,INT_MAX,1)==0);
    pid=-123;
    CHECK(posix_spawn(&pid,argv[0],&a,NULL,args,environ)==EBADF && pid==-123);
    CHECK(posix_spawn_file_actions_destroy(&a)==0);
    oldpath=getenv("PATH") ? strdup(getenv("PATH")) : NULL;
    dir=strdup(argv[0]); CHECK(dir!=NULL); slash=strrchr(dir,'/'); CHECK(slash!=NULL);
    *slash=0; CHECK(setenv("PATH",dir,1)==0);
    CHECK(posix_spawnp(&pid,slash+1,NULL,NULL,envargs,child_env)==0);
    CHECK(wait_ok(pid));
    CHECK(oldpath ? setenv("PATH",oldpath,1)==0 : unsetenv("PATH")==0);
    free(oldpath); free(dir);
    puts("PASS spawn ordered dup2/close/open, owned paths, child errors and unsupported attributes");
    return 0;
}
