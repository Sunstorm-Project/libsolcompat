/*
 * solcompat/stdlib_ext.h — Missing stdlib functions
 *
 * setenv, unsetenv, mkdtemp
 */
#ifndef SOLCOMPAT_STDLIB_EXT_H
#define SOLCOMPAT_STDLIB_EXT_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);
char *mkdtemp(char *tmpl);

/* C99 _Exit — immediate termination without atexit handlers */
void _Exit(int status);

/* C99/POSIX integer conversion */
#include <sys/types.h>
long long strtoimax(const char *nptr, char **endptr, int base);
unsigned long long strtoumax(const char *nptr, char **endptr, int base);

/*
 * Solaris 7 stdlib.h hides strtoll/strtoull/atoll behind __STDC__==0,
 * but GCC always defines __STDC__ as 1.  The functions exist in libc;
 * they just need declarations.
 */
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
long long atoll(const char *nptr);

/* C99 strtof/strtold — not in Solaris 7 libc; implemented in stdlib.c */
float strtof(const char *nptr, char **endptr);
long double strtold(const char *nptr, char **endptr);

/*
 * qsort_r — reentrant sort with caller-supplied context argument.
 *
 * Uses the GNU/glibc signature where the context pointer is the last
 * argument to both qsort_r() and the comparison function:
 *
 *   compare_fn(left_element, right_element, context_arg) → int
 *
 * Solaris 7 has qsort() but not qsort_r().  Implemented without any
 * global state so it is safe to call from multiple threads.
 */
void qsort_r(void *base, size_t element_count, size_t element_size,
             int (*compare_fn)(const void *, const void *, void *),
             void *context_arg);

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_STDLIB_EXT_H */
