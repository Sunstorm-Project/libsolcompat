/* GNU allocating getcwd extension over Solaris 7's native implementation. */
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#ifdef getcwd
#undef getcwd
#endif
/* Solaris libc exports the strong implementation behind weak getcwd. Using
 * it explicitly keeps the interposing public entry point from recursing. */
extern char *_getcwd(char *, size_t);
char *solcompat_getcwd(char *buffer, size_t size)
{
    char *allocated, *result;
    size_t capacity;
    int error;
    /* Preserve native supplied-buffer and NULL/positive-size behavior. */
    if (buffer != NULL || size != 0)
        return _getcwd(buffer, size);
    capacity = 128;
    for (;;) {
        allocated = malloc(capacity);
        if (allocated == NULL)
            return NULL;
        result = _getcwd(allocated, capacity);
        if (result != NULL)
            return result;
        error = errno;
        free(allocated);
        if (error != ERANGE) {
            errno = error;
            return NULL;
        }
        if (capacity > (size_t)-1 / 2) {
            errno = ENOMEM;
            return NULL;
        }
        capacity *= 2;
    }
}
char *getcwd(char *buffer, size_t size)
{
    return solcompat_getcwd(buffer, size);
}
