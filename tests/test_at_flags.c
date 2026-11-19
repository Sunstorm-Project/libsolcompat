#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    char dir[] = "/tmp/at-flags-XXXXXX";
    struct stat st;
    int dirfd, fd, fail = 1;
    if (!mkdtemp(dir) || chdir(dir) || (dirfd = open(".", O_RDONLY)) < 0)
        return 1;
    fd = open("target", O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (fd < 0 || close(fd) || symlink("target", "link"))
        goto done;
    fd = openat(dirfd, "target", O_RDONLY | O_CLOEXEC);
    if (fd < 0 || fcntl(fd, F_GETFD) != FD_CLOEXEC || close(fd))
        goto done;
    fd = openat(dirfd, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0 || fcntl(fd, F_GETFD) != FD_CLOEXEC || close(fd))
        goto done;
    if (fchmodat(dirfd, "link", 0777, AT_SYMLINK_NOFOLLOW) != -1 ||
        errno != ENOTSUP || stat("target", &st) || (st.st_mode & 0777) != 0600)
        goto done;
    if (unlinkat(dirfd, "target", -1) != -1 || errno != EINVAL ||
        fstatat(dirfd, "target", &st, -1) != -1 || errno != EINVAL ||
        fchownat(dirfd, "target", -1, -1, -1) != -1 || errno != EINVAL ||
        fchmodat(dirfd, "target", 0777, -1) != -1 || errno != EINVAL)
        goto done;
    if (faccessat(dirfd, "target", F_OK, AT_EACCESS) != 0 ||
        faccessat(dirfd, "target", F_OK, -1) != -1 || errno != EINVAL ||
        faccessat(dirfd, "link", F_OK, AT_SYMLINK_NOFOLLOW) != -1 ||
        errno != ENOTSUP)
        goto done;
    if (openat(dirfd, "target", O_DIRECTORY | O_RDONLY) != -1 ||
        errno != ENOTDIR || stat("target", &st) || (st.st_mode & 0777) != 0600)
        goto done;
    if (fchmodat(dirfd, "link", 0644, 0) || stat("target", &st) ||
        (st.st_mode & 0777) != 0644)
        goto done;
    fail = 0;
done:
    close(dirfd);
    unlink("link");
    unlink("target");
    chdir("/");
    rmdir(dir);
    return fail;
}
