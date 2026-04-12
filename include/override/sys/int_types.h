/*
 * override/sys/int_types.h — Intercept for Solaris 7 cross-compilation
 *
 * GCC's include-fixed/sys/int_types.h defines integer types using plain
 * 'char' for int8_t and int_least8_t.  Our override/stdint.h provides a
 * complete, C99-correct superset of those types.  This intercept file
 * prevents the include-fixed version from being included (which would
 * cause "conflicting types" errors when both headers are seen).
 *
 * When <sys/types.h> does #include <sys/int_types.h>, this file is
 * found first (via -isystem .../override), and the include-fixed
 * version is never reached.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SYS_INT_TYPES_H
#define _SYS_INT_TYPES_H

#ifdef __sun
/* All integer types are provided by override/stdint.h.
 * Include it to ensure types are available even if <stdint.h>
 * hasn't been included yet. */
#include <stdint.h>
#endif /* __sun — glibc has no sys/int_types.h; this is a no-op */

#endif /* _SYS_INT_TYPES_H */
