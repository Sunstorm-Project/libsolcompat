/*
 * memory.c — Missing memory allocation functions for Solaris 7
 *
 * posix_memalign (wrapping memalign), aligned_alloc, reallocarray,
 * MAP_ANONYMOUS wrapper via /dev/zero
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * We need the real mmap(2) and mmap64(2) — prevent the solcompat macro
 * from redirecting.  Save the real syscall symbols before including our header.
 *
 * On Solaris 7, mmap64 has signature:
 *   void *mmap64(void *, size_t, int, int, int, off64_t)
 * Programs compiled with _FILE_OFFSET_BITS=64 or _LARGEFILE64_SOURCE use
 * mmap64 instead of mmap — we must wrap both.
 */

typedef void *(*mmap_fn_t)(void *, size_t, int, int, int, long);
typedef void *(*mmap64_fn_t)(void *, size_t, int, int, int, long long);

static mmap_fn_t   real_mmap   = (mmap_fn_t)mmap;

/* mmap64 may or may not exist as a distinct symbol */
extern void *mmap64(void *, size_t, int, int, int, long long);
static mmap64_fn_t real_mmap64 = (mmap64_fn_t)mmap64;

/* Now include our header (which may redefine mmap/mmap64) */
#include "solcompat/memory.h"
/* Undo the macros for this file — we call real_mmap/real_mmap64 directly */
#undef mmap
#undef mmap64

/* Solaris 7 has memalign() in <stdlib.h> */
extern void *memalign(size_t alignment, size_t size);

int
posix_memalign(void **memptr, size_t alignment, size_t size)
{
    void *ptr;

    /* POSIX requires alignment to be power of 2 and multiple of sizeof(void*) */
    if (alignment < sizeof(void *))
        return EINVAL;
    if ((alignment & (alignment - 1)) != 0)
        return EINVAL;

    ptr = memalign(alignment, size);
    if (!ptr)
        return ENOMEM;

    *memptr = ptr;
    return 0;
}

void *
aligned_alloc(size_t alignment, size_t size)
{
    /* C11: size must be multiple of alignment */
    if (alignment == 0 || (size % alignment) != 0) {
        errno = EINVAL;
        return NULL;
    }
    return memalign(alignment, size);
}

void *
reallocarray(void *ptr, size_t nmemb, size_t size)
{
    /* Overflow check */
    if (nmemb != 0 && size != 0) {
        if (nmemb > (size_t)-1 / size) {
            errno = ENOMEM;
            return NULL;
        }
    }
    return realloc(ptr, nmemb * size);
}

/*
 * MAP_ANONYMOUS helper.
 * Solaris 7 doesn't support MAP_ANONYMOUS directly — you must
 * mmap /dev/zero.  This wrapper transparently opens /dev/zero.
 */
void *
solcompat_mmap_anon(void *addr, size_t length, int prot, int flags)
{
    int fd;
    void *result;

    fd = open("/dev/zero", O_RDWR);
    if (fd < 0)
        return MAP_FAILED;

    /* Strip our synthetic MAP_ANONYMOUS flag, use the fd */
    flags &= ~0x100;

    result = mmap(addr, length, prot, flags, fd, 0);
    close(fd);

    return result;
}

/*
 * Full mmap wrapper — intercepts MAP_ANONYMOUS.
 * If the flag is set, redirect to /dev/zero.
 * Otherwise call the real mmap(2).
 */
void *
solcompat_mmap(void *addr, size_t length, int prot, int flags,
               int fd, long offset)
{
    if (flags & 0x100) {
        /* MAP_ANONYMOUS requested — use /dev/zero approach */
        int zfd = open("/dev/zero", O_RDWR);
        void *result;
        if (zfd < 0)
            return MAP_FAILED;
        flags &= ~0x100;
        result = real_mmap(addr, length, prot, flags, zfd, 0);
        close(zfd);
        return result;
    }
    return real_mmap(addr, length, prot, flags, fd, offset);
}

/*
 * mmap64 wrapper — same logic as solcompat_mmap but for the
 * large-file-aware mmap64(2).  Programs compiled with
 * _FILE_OFFSET_BITS=64 (including GCC 11 itself) call mmap64,
 * not mmap, so this wrapper is essential.
 */
void *
solcompat_mmap64(void *addr, size_t length, int prot, int flags,
                 int fd, long long offset)
{
    if (flags & 0x100) {
        /* MAP_ANONYMOUS requested — use /dev/zero approach */
        int zfd = open("/dev/zero", O_RDWR);
        void *result;
        if (zfd < 0)
            return MAP_FAILED;
        flags &= ~0x100;
        result = real_mmap64(addr, length, prot, flags, zfd, 0);
        close(zfd);
        return result;
    }
    return real_mmap64(addr, length, prot, flags, fd, offset);
}
