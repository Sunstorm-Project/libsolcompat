/*
 * xpg.c — XPG conformance symbols for Solaris 7
 *
 * Provides __xpg6 (and __xpg4 for completeness) symbols that
 * values-xpg6.o / values-xpg4.o normally supply on later Solaris.
 *
 * GCC 11's default C11 mode links values-xpg6.o, which Solaris 7
 * doesn't have.  By providing the symbol in libsolcompat, we avoid
 * needing a separate .o stub and can eliminate the inline hack
 * from the build script.
 */

/* XPG6 conformance flag (Solaris 7 libc ignores it) */
int __xpg6 = 1;

/* XPG4 conformance flag (for completeness) */
int __xpg4 = 1;
