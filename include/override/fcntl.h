/*
 * libsolcompat -- override <fcntl.h>
 *
 * Includes the real Solaris 7 <fcntl.h> then adds POSIX constants.
 */
#ifndef _SOLCOMPAT_OVERRIDE_FCNTL_H
#define _SOLCOMPAT_OVERRIDE_FCNTL_H

#include_next <fcntl.h>

#ifdef __sun
#include <solcompat/filesystem.h>   /* POSIX_FADV_*, posix_fadvise */
#include <solcompat/at_funcs.h>     /* AT_FDCWD, AT_SYMLINK_NOFOLLOW, fstatat etc */

/* O_CLOEXEC doesn't exist on Solaris 7 -- define a placeholder
 * value so bit-test code compiles; pipe2/dup3/mkostemp implement
 * the semantics via fcntl(F_SETFD, FD_CLOEXEC). */
#ifndef O_CLOEXEC
#define O_CLOEXEC 0x800000
#endif

/* O_NOFOLLOW -- Solaris 7 lacks this; define as no-op placeholder.
 * Solaris 8+ uses 0x20000 for this. The kernel ignores unknown bits. */
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0x20000
#endif

/* O_DIRECTORY -- open must fail if path is not a directory.
 * Not present on Solaris 7; defined as no-op placeholder. */
#ifndef O_DIRECTORY
#define O_DIRECTORY 0x40000
#endif
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_FCNTL_H */
