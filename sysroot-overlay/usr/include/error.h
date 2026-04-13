/*
 * error.h — GNU error() reporting for Solaris 7
 *
 * Solaris 7 has no <error.h> and no error() / error_at_line() function.
 * bash, coreutils, gettext, and others include this header
 * unconditionally. Implementations live out-of-line in libsolcompat
 * src/error.c so they ship as real symbols in libsolcompat.so —
 * gnulib's autoconf probes `error()` by link test, and if the
 * prototype were static inline with no external symbol, gnulib
 * would compile its own replacement error.c and collide at the
 * header include site with "redefinition of 'error'". See
 * feedback_inline_only_defs_trap_gnulib.md in the project memory.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SOLCOMPAT_ERROR_H
#define _SOLCOMPAT_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

extern char *program_invocation_short_name;
extern unsigned int error_message_count;
extern int error_one_per_line;

void error(int status, int errnum, const char *fmt, ...);
void error_at_line(int status, int errnum, const char *filename,
                   unsigned int linenum, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_ERROR_H */
