/*
 * libsolcompat — full getopt / getopt_long / getopt_long_only implementation
 *
 * Solaris 7 libc provides getopt() with its own internal `optind`, stored
 * inside libc.so.1.  Any program that links libsolcompat for getopt_long
 * historically inherited that split-storage model: libsolcompat saw
 * optind@SYSVABI_1.3 from libc, while the main executable also emitted a
 * R_SPARC_COPY relocation for optind into its own .bss.  Solaris ld.so.1
 * does not always reconcile the two, so libc's internal counter advanced
 * while the executable's view of optind stayed at 1.  Symptom: binutils
 * tools print "Usage: <option(s)> <file(s)>" when given a valid file, and
 * nm prints `: '': No such file` because its own optind is stale relative
 * to libc's.
 *
 * Fix: libsolcompat owns getopt()/getopt_long()/getopt_long_only() AND
 * the optind/optarg/opterr/optopt globals end-to-end.  Consumers that link
 * libsolcompat never touch libc's getopt or its internal optind, so there
 * is exactly one copy of the state regardless of what Solaris ld.so.1
 * does with versioned-vs-unversioned symbol resolution.  libsolcompat
 * appears before libc in consumers' DT_NEEDED list (see common.sh link
 * order), so the symbols bind to libsolcompat's definitions, and the
 * executable's R_SPARC_COPY (if any) initialises from libsolcompat's
 * properly-initialised copy.
 *
 * This is a clean-room GNU-compatible implementation: supports short
 * options with optional/required arguments, clustering (-abc = -a -b -c),
 * long options with =VALUE and separate-word VALUE, optional long args,
 * and -- end-of-options marker.  POSIX-correct.  No argv permutation
 * (Solaris-style ordered scan, which is what binutils and gawk rely on).
 *
 * Copyright (c) 2024-2026 Sunstorm Project
 * SPDX-License-Identifier: MIT
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt.h"

/* Public getopt state — definitions, not just declarations.  These
 * override libc.so.1's versioned optind@SYSVABI_1.3 etc. when libsolcompat
 * appears before libc in DT_NEEDED. */
char *optarg = NULL;
int   optind = 1;   /* POSIX: first call must see optind = 1 */
int   opterr = 1;   /* Print errors by default */
int   optopt = 0;

/* Internal cursor inside a clustered short-option argv element (-abc). */
static int short_next = 0;

/*
 * getopt — POSIX short-option parser, reimplemented locally so we do not
 * share optind state with libc.so.1's internal getopt.
 *
 * Return values:
 *   option character on success
 *   '?'     on unknown option or missing argument (when optstring[0] != ':')
 *   ':'     on missing argument when optstring[0] == ':'
 *   -1      at end of options
 */
int
getopt(int argc, char *const argv[], const char *optstring)
{
    const char *arg;
    const char *spec;
    char        opt;

    if (optstring == NULL)
        optstring = "";

    /* Continuing a cluster like -abc from a previous call */
    if (short_next > 0) {
        if (optind >= argc || argv[optind] == NULL) {
            short_next = 0;
            return -1;
        }
        arg = argv[optind];
        opt = arg[short_next];
        if (opt == '\0') {
            /* End of this cluster — advance */
            short_next = 0;
            optind++;
            return getopt(argc, argv, optstring);
        }
    } else {
        if (optind >= argc || argv[optind] == NULL)
            return -1;
        arg = argv[optind];
        if (arg[0] != '-' || arg[1] == '\0')
            return -1;  /* Non-option — stop scanning */
        if (arg[0] == '-' && arg[1] == '-' && arg[2] == '\0') {
            optind++;
            return -1;  /* "--" marker */
        }
        short_next = 1;
        opt = arg[1];
    }

    /* Look up opt in optstring */
    spec = strchr(optstring, opt);
    if (spec == NULL || opt == ':') {
        if (opterr && optstring[0] != ':')
            fprintf(stderr, "%s: invalid option -- '%c'\n",
                    argv[0] ? argv[0] : "", opt);
        optopt = opt;
        short_next++;
        if (argv[optind] != NULL && argv[optind][short_next] == '\0') {
            short_next = 0;
            optind++;
        }
        return '?';
    }

    if (spec[1] == ':') {
        /* Option takes an argument */
        if (argv[optind][short_next + 1] != '\0') {
            /* -oARG form */
            optarg = (char *)&argv[optind][short_next + 1];
            short_next = 0;
            optind++;
        } else if (spec[2] == ':') {
            /* -o :: — optional argument, not present inline */
            optarg = NULL;
            short_next = 0;
            optind++;
        } else {
            /* -o ARG form — next argv is the argument */
            short_next = 0;
            optind++;
            if (optind >= argc) {
                if (opterr && optstring[0] != ':')
                    fprintf(stderr,
                            "%s: option requires an argument -- '%c'\n",
                            argv[0] ? argv[0] : "", opt);
                optopt = opt;
                return (optstring[0] == ':') ? ':' : '?';
            }
            optarg = argv[optind++];
        }
    } else {
        /* No argument */
        optarg = NULL;
        short_next++;
        if (argv[optind][short_next] == '\0') {
            short_next = 0;
            optind++;
        }
    }
    return opt;
}

/*
 * getopt_long — handle --long options directly; short options via local
 * getopt() above (NOT libc).  Clean-room GNU-compatible.
 */
int
getopt_long(int argc, char *const argv[], const char *optstring,
            const struct option *longopts, int *longindex)
{
    const char *arg;
    int i;

    if (optind >= argc || argv[optind] == NULL)
        return -1;

    arg = argv[optind];

    /* If in the middle of a short-option cluster, stay in getopt */
    if (short_next > 0)
        return getopt(argc, argv, optstring);

    /* "--" end-of-options marker */
    if (arg[0] == '-' && arg[1] == '-' && arg[2] == '\0') {
        optind++;
        return -1;
    }

    /* Long option (--xxx) */
    if (arg[0] == '-' && arg[1] == '-' && arg[2] != '\0' && longopts != NULL) {
        const char *name = arg + 2;
        const char *eq = strchr(name, '=');
        size_t namelen = eq ? (size_t)(eq - name) : strlen(name);
        int match = -1;
        int ambiguous = 0;

        for (i = 0; longopts[i].name != NULL; i++) {
            if (strncmp(longopts[i].name, name, namelen) == 0) {
                if (strlen(longopts[i].name) == namelen) {
                    /* Exact match wins */
                    match = i;
                    ambiguous = 0;
                    break;
                }
                if (match >= 0) {
                    ambiguous = 1;
                } else {
                    match = i;
                }
            }
        }

        if (ambiguous) {
            if (opterr)
                fprintf(stderr, "%s: option '--%.*s' is ambiguous\n",
                        argv[0] ? argv[0] : "", (int)namelen, name);
            optind++;
            return '?';
        }

        if (match < 0) {
            if (opterr)
                fprintf(stderr, "%s: unrecognized option '--%.*s'\n",
                        argv[0] ? argv[0] : "", (int)namelen, name);
            optind++;
            return '?';
        }

        if (longindex)
            *longindex = match;
        optind++;

        if (longopts[match].has_arg == no_argument) {
            if (eq) {
                if (opterr)
                    fprintf(stderr,
                            "%s: option '--%s' doesn't allow an argument\n",
                            argv[0] ? argv[0] : "", longopts[match].name);
                return '?';
            }
            optarg = NULL;
        } else if (longopts[match].has_arg == required_argument) {
            if (eq) {
                optarg = (char *)(eq + 1);
            } else if (optind < argc) {
                optarg = argv[optind++];
            } else {
                if (opterr)
                    fprintf(stderr,
                            "%s: option '--%s' requires an argument\n",
                            argv[0] ? argv[0] : "", longopts[match].name);
                return (optstring && optstring[0] == ':') ? ':' : '?';
            }
        } else {
            optarg = eq ? (char *)(eq + 1) : NULL;
        }

        if (longopts[match].flag) {
            *longopts[match].flag = longopts[match].val;
            return 0;
        }
        return longopts[match].val;
    }

    /* Not a long option — short option or non-option */
    return getopt(argc, argv, optstring);
}

/*
 * getopt_long_only — like getopt_long but single-dash args may be long
 * options too (e.g. gcc's -static, -shared).
 */
int
getopt_long_only(int argc, char *const argv[], const char *optstring,
                 const struct option *longopts, int *longindex)
{
    const char *arg;
    int i;

    if (optind >= argc || argv[optind] == NULL)
        return -1;

    arg = argv[optind];

    if (short_next > 0)
        return getopt(argc, argv, optstring);

    if (arg[0] == '-' && arg[1] == '-' && arg[2] == '\0') {
        optind++;
        return -1;
    }

    /* "--foo" — pure long option path */
    if (arg[0] == '-' && arg[1] == '-')
        return getopt_long(argc, argv, optstring, longopts, longindex);

    /* "-foo" with foo.len >= 2 — try long option first */
    if (arg[0] == '-' && arg[1] != '\0' && arg[2] != '\0' &&
        longopts != NULL) {
        const char *name = arg + 1;
        const char *eq = strchr(name, '=');
        size_t namelen = eq ? (size_t)(eq - name) : strlen(name);
        int match = -1;

        for (i = 0; longopts[i].name != NULL; i++) {
            if (strncmp(longopts[i].name, name, namelen) == 0 &&
                strlen(longopts[i].name) == namelen) {
                match = i;
                break;
            }
        }

        if (match >= 0) {
            if (longindex)
                *longindex = match;
            optind++;

            if (longopts[match].has_arg == required_argument) {
                if (eq) {
                    optarg = (char *)(eq + 1);
                } else if (optind < argc) {
                    optarg = argv[optind++];
                } else {
                    if (opterr)
                        fprintf(stderr,
                                "%s: option '-%s' requires an argument\n",
                                argv[0] ? argv[0] : "", longopts[match].name);
                    return (optstring && optstring[0] == ':') ? ':' : '?';
                }
            } else if (longopts[match].has_arg == optional_argument) {
                optarg = eq ? (char *)(eq + 1) : NULL;
            } else {
                if (eq) {
                    if (opterr)
                        fprintf(stderr,
                                "%s: option '-%s' doesn't allow an argument\n",
                                argv[0] ? argv[0] : "", longopts[match].name);
                    return '?';
                }
                optarg = NULL;
            }

            if (longopts[match].flag) {
                *longopts[match].flag = longopts[match].val;
                return 0;
            }
            return longopts[match].val;
        }
        /* Fall through to short-option processing */
    }

    return getopt(argc, argv, optstring);
}
