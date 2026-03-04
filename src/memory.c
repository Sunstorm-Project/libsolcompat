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
 * We need the real mmap(2) and mmap64(2) for our wrappers.
 *
 * Two usage modes:
 * 1. Compile-time: macros redirect mmap/mmap64 → solcompat_mmap/solcompat_mmap64
 * 2. Runtime (LD_PRELOAD): we export mmap/mmap64 symbols that interpose the
 *    libc versions.  We use dlsym(RTLD_NEXT) to find the originals.
 *
 * On Solaris 7, mmap64 has signature:
 *   caddr_t mmap64(caddr_t, size_t, int, int, int, off64_t)
 * Programs compiled with _FILE_OFFSET_BITS=64 or _LARGEFILE64_SOURCE use
 * mmap64 instead of mmap — we must wrap both.
 */

#include <dlfcn.h>

typedef void *(*mmap_fn_t)(void *, size_t, int, int, int, long);
typedef void *(*mmap64_fn_t)(void *, size_t, int, int, int, long long);

static mmap_fn_t   orig_mmap   = NULL;
static mmap64_fn_t orig_mmap64 = NULL;

static void
init_real_mmap(void)
{
    if (!orig_mmap)
        orig_mmap = (mmap_fn_t)dlsym(RTLD_NEXT, "mmap");
    if (!orig_mmap64)
        orig_mmap64 = (mmap64_fn_t)dlsym(RTLD_NEXT, "mmap64");
}

/* Now include our header (which may redefine mmap/mmap64) */
#include "solcompat/memory.h"
/* Undo the macros for this file — we call orig_mmap/orig_mmap64 directly */
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

    init_real_mmap();
    fd = open("/dev/zero", O_RDWR);
    if (fd < 0)
        return MAP_FAILED;

    /* Strip our synthetic MAP_ANONYMOUS flag, use the fd */
    flags &= ~0x100;

    result = orig_mmap(addr, length, prot, flags, fd, 0);
    close(fd);

    return result;
}

/*
 * Internal helper: perform MAP_ANONYMOUS → /dev/zero for mmap.
 */
static void *
do_mmap_anon_fixup(void *addr, size_t length, int prot, int flags,
                   int fd, long offset)
{
    init_real_mmap();
    if (flags & 0x100) {
        int zfd = open("/dev/zero", O_RDWR);
        void *result;
        if (zfd < 0)
            return MAP_FAILED;
        flags &= ~0x100;
        result = orig_mmap(addr, length, prot, flags, zfd, 0);
        close(zfd);
        return result;
    }
    return orig_mmap(addr, length, prot, flags, fd, offset);
}

/*
 * Internal helper: perform MAP_ANONYMOUS → /dev/zero for mmap64.
 */
static void *
do_mmap64_anon_fixup(void *addr, size_t length, int prot, int flags,
                     int fd, long long offset)
{
    init_real_mmap();
    if (flags & 0x100) {
        int zfd = open("/dev/zero", O_RDWR);
        void *result;
        if (zfd < 0)
            return MAP_FAILED;
        flags &= ~0x100;
        result = orig_mmap64(addr, length, prot, flags, zfd, 0);
        close(zfd);
        return result;
    }
    return orig_mmap64(addr, length, prot, flags, fd, offset);
}

/*
 * solcompat_mmap / solcompat_mmap64 — compile-time macro targets.
 * Code compiled with solcompat headers calls these via the mmap/mmap64 macros.
 */
void *
solcompat_mmap(void *addr, size_t length, int prot, int flags,
               int fd, long offset)
{
    return do_mmap_anon_fixup(addr, length, prot, flags, fd, offset);
}

void *
solcompat_mmap64(void *addr, size_t length, int prot, int flags,
                 int fd, long long offset)
{
    return do_mmap64_anon_fixup(addr, length, prot, flags, fd, offset);
}

/*
 * LD_PRELOAD interposition symbols.
 * When this .so is loaded via LD_PRELOAD, these replace libc's mmap/mmap64.
 * dlsym(RTLD_NEXT) finds the real libc versions.
 *
 * On Solaris 7 these use caddr_t (char *) returns and off_t/off64_t offsets,
 * but the ABI is the same as void* returns — the interposition works.
 */
void *
mmap(void *addr, size_t length, int prot, int flags, int fd, long offset)
{
    return do_mmap_anon_fixup(addr, length, prot, flags, fd, offset);
}

void *
mmap64(void *addr, size_t length, int prot, int flags, int fd, long long offset)
{
    return do_mmap64_anon_fixup(addr, length, prot, flags, fd, offset);
}
