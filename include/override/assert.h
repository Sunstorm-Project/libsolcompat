/*
 * override/assert.h — C11 static_assert for Solaris 7
 *
 * Solaris 7 assert.h predates C11 and does not define static_assert.
 * C11 requires assert.h to provide it as a macro for _Static_assert.
 *
 * NOTE: No include guard — the C standard requires assert.h to be
 * re-includable so it can redefine assert based on NDEBUG state.
 *
 * Part of libsolcompat
 */

/* Pull in the real Solaris 7 assert.h (redefines assert each time) */
#include_next <assert.h>

#ifdef __sun
/* C11 7.2: static_assert expands to _Static_assert */
#if !defined(__cplusplus) && !defined(static_assert)
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#    define static_assert _Static_assert
#  endif
#endif
#endif /* __sun */
