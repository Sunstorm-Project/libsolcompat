/* Compile the actual Solaris ILP32 stat call with the caller's layout. */
#undef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 32
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "statat_internal.h"

#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 0x100
#endif

static int
stat_small(const char *path, void *buf, int flags)
{
    return flags & AT_SYMLINK_NOFOLLOW ? lstat(path, buf) : stat(path, buf);
}

int
solcompat_fstatat32(int dirfd, const char *path, struct stat *buf, int flags)
{
    return solcompat_statat_with(dirfd, path, buf, flags, stat_small);
}
