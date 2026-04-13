/*
 * error.h — GNU error() reporting for Solaris 7
 *
 * Solaris 7 has no <error.h> and no error() / error_at_line() function.
 * bash / coreutils / gettext include this header unconditionally.
 * Inline implementations: print program name + formatted message to
 * stderr, optionally exit with the given status. Matches glibc
 * semantics for non-zero status.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SOLCOMPAT_ERROR_H
#define _SOLCOMPAT_ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char *program_invocation_short_name;
extern unsigned int error_message_count;
extern int error_one_per_line;

static inline void
error(int status, int errnum, const char *fmt, ...)
{
    va_list ap;
    fflush(stdout);
    if (program_invocation_short_name)
        fprintf(stderr, "%s: ", program_invocation_short_name);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (errnum)
        fprintf(stderr, ": %s", strerror(errnum));
    fputc('\n', stderr);
    error_message_count++;
    if (status)
        exit(status);
}

static inline void
error_at_line(int status, int errnum, const char *filename,
              unsigned int linenum, const char *fmt, ...)
{
    va_list ap;
    fflush(stdout);
    if (program_invocation_short_name)
        fprintf(stderr, "%s:", program_invocation_short_name);
    if (filename)
        fprintf(stderr, "%s:%u: ", filename, linenum);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (errnum)
        fprintf(stderr, ": %s", strerror(errnum));
    fputc('\n', stderr);
    error_message_count++;
    if (status)
        exit(status);
}

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_ERROR_H */
