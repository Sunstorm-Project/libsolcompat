#ifndef SOLCOMPAT_DIRFD_INTERNAL_H
#define SOLCOMPAT_DIRFD_INTERNAL_H
int solcompat_with_dirfd(int fd, const char *path,
                        int (*operation)(const char *, void *), void *argument);
#endif
