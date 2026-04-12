/*
 * stdbool.h — C99 boolean type for Solaris 7
 *
 * Solaris 7's /usr/include has no <stdbool.h>.  Packages that require
 * C99 (autoconf feature tests, GCC internals, cmake configure checks)
 * fail with "stdbool.h: No such file or directory".
 *
 * This header is installed into the sysroot-overlay so it appears at
 * /usr/include/stdbool.h inside the build container, satisfying both
 * cross-compile and canadian-cross configure checks without patching
 * each package individually.
 *
 * In C++, bool/true/false are built-in keywords so the macros are
 * intentionally suppressed.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _STDBOOL_H
#define _STDBOOL_H

#ifndef __cplusplus
#define bool  _Bool
#define true  1
#define false 0
#endif /* __cplusplus */

#define __bool_true_false_are_defined 1

#endif /* _STDBOOL_H */
