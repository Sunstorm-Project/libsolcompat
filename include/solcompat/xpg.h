/*
 * solcompat/xpg.h — XPG conformance symbols for Solaris 7
 *
 * GCC 11 defaults to C11/C17, which on Solaris links values-xpg6.o
 * for XPG6 conformance.  Solaris 7 only ships values-xpg4.o.
 *
 * The __xpg6 symbol controls conformance behavior in Solaris libc
 * (e.g., printf format handling).  Solaris 7's libc doesn't check
 * __xpg6, so setting it to 1 is harmless and prevents link errors.
 *
 * Similarly, __xpg4 controls XPG4 mode in Solaris 7 libc.
 */
#ifndef SOLCOMPAT_XPG_H
#define SOLCOMPAT_XPG_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * XPG conformance flag.  Solaris libc checks this to decide between
 * XPG6 (C99/SUSv3) and legacy behavior.  On Solaris 7, libc ignores
 * it, but we need the symbol to satisfy the linker when GCC specs
 * add values-xpg6.o to the link.
 */
extern int __xpg6;

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_XPG_H */
