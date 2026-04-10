/*
 * override/wctype.h — POSIX.1-2001 iswblank for Solaris 7
 *
 * Solaris 7's <wctype.h> ships only the C90 wide-char predicates
 * (iswalpha, iswspace, ...). POSIX 2001 added iswblank, which gnulib
 * modules (wordwrap.c, regex.c, ...) reference at the link site even
 * when the system header is silent.
 *
 * Part of libsolcompat
 */

/* Pull in the real Solaris 7 <wctype.h> first */
#include_next <wctype.h>

#ifndef _SOLCOMPAT_OVERRIDE_WCTYPE_H_DECLS
#define _SOLCOMPAT_OVERRIDE_WCTYPE_H_DECLS

#ifdef __cplusplus
extern "C" {
#endif

/* POSIX.1-2001 wide-char blank predicate (libsolcompat-provided) */
int iswblank(wint_t wc);

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_OVERRIDE_WCTYPE_H_DECLS */
