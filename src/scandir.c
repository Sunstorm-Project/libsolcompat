/* Solaris ILP32 dirent and dirent64 differ in the position of d_name.
 * Compile the same implementation for each public callback and result ABI. */
#undef _FILE_OFFSET_BITS
#ifdef SOLCOMPAT_SMALL_DIRENT
#define _FILE_OFFSET_BITS 32
#else
#define _FILE_OFFSET_BITS 64
#endif
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#undef alphasort
#undef scandir
#ifdef SOLCOMPAT_SMALL_DIRENT
#define scandir solcompat_scandir32
#define alphasort solcompat_alphasort32
#endif

int scandir(const char *path, struct dirent ***result,
            int (*select_entry)(const struct dirent *),
            int (*compare)(const struct dirent **, const struct dirent **))
{
    DIR *dir = opendir(path);
    struct dirent **list = NULL;
    size_t count = 0, capacity = 0;
    int error;
    if (!dir)
        return -1;
    for (;;) {
        struct dirent *entry;
        errno = 0;
        entry = readdir(dir);
        if (!entry) {
            if (errno)
                goto fail;
            break;
        }
        if (select_entry && !select_entry(entry))
            continue;
        if (count == capacity) {
            if (capacity > INT_MAX / 2 || capacity > SIZE_MAX / sizeof(*list) / 2) {
                errno = EOVERFLOW;
                goto fail;
            }
            capacity = capacity ? capacity * 2 : 32;
            struct dirent **grown = realloc(list, capacity * sizeof(*list));
            if (!grown)
                goto fail;
            list = grown;
        }
        list[count] = malloc(entry->d_reclen);
        if (!list[count])
            goto fail;
        memcpy(list[count++], entry, entry->d_reclen);
    }
    closedir(dir);
    if (compare && count > 1)
        qsort(list, count, sizeof(*list), (int (*)(const void *, const void *))compare);
    *result = list;
    return (int)count;

fail:
    error = errno;
    closedir(dir);
    while (count)
        free(list[--count]);
    free(list);
    errno = error;
    return -1;
}

int alphasort(const struct dirent **a, const struct dirent **b)
{
    return strcoll((*a)->d_name, (*b)->d_name);
}
