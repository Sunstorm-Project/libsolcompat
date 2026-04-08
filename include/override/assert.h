/*
 * override/assert.h — C11 static_assert for Solaris 7
 *
 * Solaris 7 assert.h predates C11 and does not define static_assert.
 * C11 requires assert.h to provide it as a macro for _Static_assert.
 *
 * Part of libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_ASSERT_H
#define _SOLCOMPAT_OVERRIDE_ASSERT_H

/* Pull in the real Solaris 7 assert.h */
#include_next <assert.h>

/* C11 7.2: static_assert expands to _Static_assert */
#if !defined(__cplusplus) && !defined(static_assert)
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#    define static_assert _Static_assert
#  endif
#endif

#endif /* _SOLCOMPAT_OVERRIDE_ASSERT_H */
