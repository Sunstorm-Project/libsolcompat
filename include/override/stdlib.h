/*
 * override/stdlib.h — C99/C11 stdlib extensions for Solaris 7
 *
 * Solaris 7's stdlib.h lacks functions added in later POSIX revisions:
 * posix_memalign, mkdtemp, realpath with NULL, etc.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_STDLIB_H
#define _SOLCOMPAT_OVERRIDE_STDLIB_H

/* Pull in the real Solaris 7 /usr/include/stdlib.h */
#include_next <stdlib.h>

#ifdef __sun
#include <solcompat/stdlib_ext.h>

/* Memory allocation extensions from solcompat/memory.h.
   We declare them directly instead of #include <solcompat/memory.h>
   because that header pulls in <sys/mman.h> which defines MAP_PRIVATE
   and other macros that conflict with application code (e.g. OpenSSL). */
#ifdef __cplusplus
extern "C" {
#endif
int posix_memalign(void **memptr, size_t alignment, size_t size);
void *aligned_alloc(size_t alignment, size_t size);
void *reallocarray(void *ptr, size_t nmemb, size_t size);
#ifdef __cplusplus
}
#endif

/* BSD/modern arc4random family — many programs expect these in <stdlib.h> */
#include <solcompat/random.h>

/*
 * POSIX.1 declares environ in <unistd.h>, but many programs (and cmake's
 * feature-detection) expect it in <stdlib.h>.  Provide it here so that
 * "#include <stdlib.h>" is sufficient.
 */
#ifdef __cplusplus
extern "C" {
#endif
extern char **environ;
#ifdef __cplusplus
}
#endif
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_STDLIB_H */
