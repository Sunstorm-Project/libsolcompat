/*
 * libsolcompat — getopt_long() and getopt_long_only() implementation
 *
 * Solaris 7 has getopt() in libc but lacks getopt_long().
 * This is a clean-room implementation compatible with GNU getopt_long.
 *
 * Copyright (c) 2024-2026 Sunstorm Project
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Pull in the struct option definition — sysroot-overlay provides getopt.h */
#include "getopt.h"

/*
 * getopt_long — parse long and short options
 *
 * This wraps the system getopt() for short options and handles
 * --long-option and --long-option=value forms directly.
 */

/* Internal state */
static int _sol_optcharind = 0;  /* index within a multi-short-opt cluster */

int
getopt_long(int argc, char *const argv[], const char *optstring,
            const struct option *longopts, int *longindex)
{
    const char *arg;
    int i;

    if (optind >= argc || argv[optind] == NULL)
        return -1;

    arg = argv[optind];

    /* Check for "--" end-of-options marker */
    if (strcmp(arg, "--") == 0) {
        optind++;
        return -1;
    }

    /* Check for long option (--xxx) */
    if (arg[0] == '-' && arg[1] == '-' && arg[2] != '\0') {
        const char *name = arg + 2;
        const char *eq = strchr(name, '=');
        size_t namelen = eq ? (size_t)(eq - name) : strlen(name);
        int match = -1;
        int ambiguous = 0;

        /* Search for matching long option */
        for (i = 0; longopts[i].name != NULL; i++) {
            if (strncmp(longopts[i].name, name, namelen) == 0) {
                if (strlen(longopts[i].name) == namelen) {
                    /* Exact match */
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
                        argv[0], (int)namelen, name);
            optind++;
            return '?';
        }

        if (match < 0) {
            if (opterr)
                fprintf(stderr, "%s: unrecognized option '--%.*s'\n",
                        argv[0], (int)namelen, name);
            optind++;
            return '?';
        }

        /* Found a match */
        if (longindex)
            *longindex = match;

        optind++;

        if (longopts[match].has_arg == no_argument) {
            if (eq) {
                if (opterr)
                    fprintf(stderr,
                            "%s: option '--%s' doesn't allow an argument\n",
                            argv[0], longopts[match].name);
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
                            argv[0], longopts[match].name);
                return (optstring[0] == ':') ? ':' : '?';
            }
        } else { /* optional_argument */
            optarg = eq ? (char *)(eq + 1) : NULL;
        }

        if (longopts[match].flag) {
            *longopts[match].flag = longopts[match].val;
            return 0;
        }
        return longopts[match].val;
    }

    /* Not a long option — delegate to system getopt() */
    return getopt(argc, argv, optstring);
}

/*
 * getopt_long_only — like getopt_long but also tries long options
 * for single-dash arguments (-option).
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

    /* Check for "--" end-of-options marker */
    if (strcmp(arg, "--") == 0) {
        optind++;
        return -1;
    }

    /* For single-dash arguments longer than 2 chars, try as long option first */
    if (arg[0] == '-' && arg[1] != '-' && arg[1] != '\0' && arg[2] != '\0') {
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
            /* Treat as long option */
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
                                argv[0], longopts[match].name);
                    return (optstring[0] == ':') ? ':' : '?';
                }
            } else if (longopts[match].has_arg == optional_argument) {
                optarg = eq ? (char *)(eq + 1) : NULL;
            } else {
                if (eq) {
                    if (opterr)
                        fprintf(stderr,
                                "%s: option '-%s' doesn't allow an argument\n",
                                argv[0], longopts[match].name);
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
        /* No long match — fall through to short option processing */
    }

    /* Handle as long option if it starts with -- */
    if (arg[0] == '-' && arg[1] == '-')
        return getopt_long(argc, argv, optstring, longopts, longindex);

    /* Short option — use system getopt */
    return getopt(argc, argv, optstring);
}
