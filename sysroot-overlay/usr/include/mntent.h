/*
 * mntent.h — GNU mount-table access for Solaris 7
 *
 * Solaris 7 uses <sys/mnttab.h> with struct mnttab and getmntent()
 * returning int (not FILE*-based). coreutils/glib/libffi include
 * <mntent.h> expecting the glibc API (struct mntent, setmntent()
 * returning FILE*, getmntent() returning struct mntent*, endmntent()).
 * libsolcompat wrappers in src/filesystem.c bridge the two APIs.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SOLCOMPAT_MNTENT_H
#define _SOLCOMPAT_MNTENT_H

#include <stdio.h>

#define MOUNTED    "/etc/mnttab"
#define _PATH_MOUNTED MOUNTED

#ifdef __cplusplus
extern "C" {
#endif

struct mntent {
    char *mnt_fsname;   /* device or server */
    char *mnt_dir;      /* mount point */
    char *mnt_type;     /* filesystem type */
    char *mnt_opts;     /* mount options */
    int   mnt_freq;     /* dump frequency (unused on Solaris) */
    int   mnt_passno;   /* fsck pass (unused on Solaris) */
};

FILE          *setmntent(const char *filename, const char *type);
struct mntent *getmntent(FILE *fp);
int            addmntent(FILE *fp, const struct mntent *mnt);
int            endmntent(FILE *fp);
char          *hasmntopt(const struct mntent *mnt, const char *opt);

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_MNTENT_H */
