/*
 * error.c — GNU error() / error_at_line() for Solaris 7
 *
 * Out-of-line definitions so these ship as external symbols in
 * libsolcompat.so. gnulib's autoconf probes `error()` via a link
 * test; if the symbol isn't in the library, gnulib builds its own
 * replacement lib/error.c, which then #includes <error.h> and
 * collides with any inline definition there. Keeping the bodies
 * here lets gnulib's link probe succeed and its replacement is
 * suppressed.
 *
 * Semantics: print program name + formatted message to stderr,
 * optionally append strerror(errnum), exit with `status` if non-zero.
 * Matches glibc's <error.h> behavior closely enough for the callers
 * we see (bash, coreutils, gettext, gnulib consumers).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

char *program_invocation_short_name = NULL;
unsigned int error_message_count = 0;
int error_one_per_line = 0;

void
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

void
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
