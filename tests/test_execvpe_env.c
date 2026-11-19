#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <solcompat/process.h>

int main(void)
{
    char dir[] = "/tmp/execvpe-XXXXXX";
    char *args[] = {"tool", "argument", NULL};
    char *env[] = {"PATH=/no-such-path", "MARK=child", NULL};
    FILE *f;
    int status;
    pid_t pid;
    if (!mkdtemp(dir) || chdir(dir) || !(f = fopen("tool", "w")))
        return 1;
    if (fputs("test \"$MARK:$1:$PATH\" = child:argument:/no-such-path\n", f) < 0 ||
        fclose(f) || chmod("tool", 0700) || setenv("PATH", "", 1))
        return 2;
    if ((pid = fork()) < 0)
        return 3;
    if (pid == 0) {
        execvpe("tool", args, env);
        _exit(42);
    }
    if (waitpid(pid, &status, 0) != pid || !WIFEXITED(status) || WEXITSTATUS(status))
        return 4;
    if (execvpe("", args, env) != -1 || errno != ENOENT)
        return 5;
    unlink("tool");
    chdir("/");
    return rmdir(dir) != 0;
}
