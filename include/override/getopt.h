/*
 * libsolcompat — <getopt.h>
 *
 * Solaris 7 declares getopt() in <stdlib.h> and <unistd.h> but does
 * not provide <getopt.h> or getopt_long()/getopt_long_only().
 *
 * This header provides the full GNU-compatible getopt interface.
 */
#ifndef _SOLCOMPAT_GETOPT_H
#define _SOLCOMPAT_GETOPT_H

#ifdef __sun

#ifdef __cplusplus
extern "C" {
#endif

/* Standard getopt — already in libc, just need declarations */
extern char *optarg;
extern int optind, opterr, optopt;

extern int getopt(int argc, char *const argv[], const char *optstring);

/* Extended GNU getopt_long */
struct option {
    const char *name;   /* Long option name */
    int         has_arg; /* no_argument, required_argument, optional_argument */
    int        *flag;   /* If non-NULL, set *flag to val and return 0 */
    int         val;    /* Value to return (or store in *flag) */
};

#define no_argument        0
#define required_argument  1
#define optional_argument  2

extern int getopt_long(int argc, char *const argv[],
                       const char *optstring,
                       const struct option *longopts,
                       int *longindex);

extern int getopt_long_only(int argc, char *const argv[],
                            const char *optstring,
                            const struct option *longopts,
                            int *longindex);

#ifdef __cplusplus
}
#endif

#else /* !__sun */
#include_next <getopt.h>
#endif /* __sun */

#endif /* _SOLCOMPAT_GETOPT_H */
