/*
 * memstream.c — fmemopen and open_memstream for Solaris 7
 *
 * fmemopen:
 *   Opens /dev/null to obtain a real fd, uses fdopen() to get a properly
 *   initialised Solaris FILE slot, then redirects the internal buffer
 *   pointers to the caller's memory.  For "r" mode the pre-filled _cnt
 *   causes stdio to serve bytes directly from the buffer; when exhausted,
 *   read() on the /dev/null fd returns 0 (EOF).  For "w" mode writes land
 *   in the caller's buffer; fflush/fclose flush a copy to /dev/null so the
 *   data remains in the buffer.
 *
 * open_memstream:
 *   Backed by a tmpfile().  A process-wide registry maps each FILE* to the
 *   caller's {char **ptr, size_t *sizeloc}.  fclose() and fflush() are
 *   interposed via RTLD_NEXT: on every flush or close the tmpfile is read
 *   back into a malloc'd buffer and *ptr / *sizeloc are updated before the
 *   real libc function is called.
 *
 * Threading:
 *   The registry is protected by a mutex.  fclose() on the same FILE* from
 *   two threads concurrently is undefined behaviour (same as libc), so no
 *   extra protection is needed for the sync/free sequence.
 *
 * Portability note:
 *   Programmes using open_memstream must link with -ldl (needed for dlsym).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>

/* Solaris 7 FILE struct layout (32-bit) — see /usr/include/stdio_impl.h */
#include <stdio_impl.h>

/* Flags from <stdio.h> */
#ifndef _IOREAD
#  define _IOREAD   0001
#endif
#ifndef _IOWRT
#  define _IOWRT    0002
#endif
#ifndef _IOMYBUF
#  define _IOMYBUF  0010
#endif
#ifndef _NFILE
#  define _NFILE    20      /* SPARC ABI static FILE slot count */
#endif

/*
 * _bufendtab[fd] holds a pointer to one-past-the-end of the stdio buffer
 * for file descriptor fd (when fd < _NFILE).  Declared in <stdio.h>.
 */
extern unsigned char *_bufendtab[];

/* =========================================================================
 * fmemopen — memory-backed FILE stream
 * ========================================================================= */

FILE *
fmemopen(void *buf, size_t size, const char *mode)
{
    FILE          *fp;
    int            devnull_fd;
    const char    *fdopen_mode;

    if (buf == NULL || size == 0 || mode == NULL) {
        errno = EINVAL;
        return NULL;
    }

    if (mode[0] == 'r') {
        devnull_fd  = open("/dev/null", O_RDONLY);
        fdopen_mode = "r";
    } else if (mode[0] == 'w') {
        devnull_fd  = open("/dev/null", O_WRONLY);
        fdopen_mode = "w";
        /* POSIX: initial content undefined; NUL-fill for predictability */
        memset(buf, 0, size);
    } else if (mode[0] == 'a') {
        devnull_fd  = open("/dev/null", O_WRONLY);
        fdopen_mode = "a";
    } else {
        errno = EINVAL;
        return NULL;
    }

    if (devnull_fd < 0)
        return NULL;

    fp = fdopen(devnull_fd, fdopen_mode);
    if (fp == NULL) {
        close(devnull_fd);
        return NULL;
    }

    /*
     * Redirect stdio's internal buffer to the caller's memory.
     * _cnt  = bytes available (read) / remaining space (write)
     * _ptr  = current position within the buffer
     * _base = start of buffer
     */
    fp->_base = (unsigned char *)buf;
    fp->_ptr  = (unsigned char *)buf;
    fp->_cnt  = (ssize_t)size;

    /*
     * Clear _IOMYBUF so that fclose() does not attempt to free the
     * caller's buffer.
     */
    fp->_flag = (unsigned char)(fp->_flag & ~_IOMYBUF);

    /*
     * Keep _bufendtab consistent.  It is indexed by fd and records the
     * one-past-end address of the buffer — used internally by _bufend()
     * and _bufsiz() macros.
     */
    if (devnull_fd < _NFILE)
        _bufendtab[devnull_fd] = (unsigned char *)buf + size;

    return fp;
}

/* =========================================================================
 * open_memstream — dynamically-growing write-only memory stream
 * ========================================================================= */

struct memstream_entry {
    FILE                   *fp;
    char                  **ptr;
    size_t                 *sizeloc;
    struct memstream_entry *next;
};

static struct memstream_entry *memstream_list        = NULL;
static pthread_mutex_t         memstream_list_mutex  = PTHREAD_MUTEX_INITIALIZER;

/* Pointers to the real libc functions, resolved once via RTLD_NEXT. */
static int (*real_fclose)(FILE *)     = NULL;
static int (*real_fflush)(FILE *)     = NULL;

static void
ensure_real_fns(void)
{
    if (real_fclose == NULL)
        real_fclose = (int (*)(FILE *))dlsym(RTLD_NEXT, "fclose");
    if (real_fflush == NULL)
        real_fflush = (int (*)(FILE *))dlsym(RTLD_NEXT, "fflush");
}

/*
 * Drain the tmpfile into a fresh malloc'd buffer and update the caller's
 * *ptr / *sizeloc.  Calls the real fflush to push any buffered data to the
 * tmpfile before reading it back.  Leaves the file position at the end so
 * subsequent writes continue to append.
 */
static void
memstream_sync(FILE *fp, struct memstream_entry *entry)
{
    long   byte_count;
    char  *new_buf;

    ensure_real_fns();

    /* Flush any data still buffered in stdio into the underlying tmpfile */
    real_fflush(fp);

    byte_count = ftell(fp);
    if (byte_count < 0)
        return;

    new_buf = (char *)malloc((size_t)byte_count + 1);
    if (new_buf == NULL)
        return;

    rewind(fp);
    if (fread(new_buf, 1, (size_t)byte_count, fp) != (size_t)byte_count) {
        free(new_buf);
        return;
    }
    /* Restore write position to the end for further writes */
    fseek(fp, byte_count, SEEK_SET);

    new_buf[byte_count] = '\0';

    free(*entry->ptr);
    *entry->ptr     = new_buf;
    *entry->sizeloc = (size_t)byte_count;
}

/* Find and remove an entry from the registry; returns it (caller must free). */
static struct memstream_entry *
memstream_remove(FILE *fp)
{
    struct memstream_entry **pprev;
    struct memstream_entry  *entry;

    pthread_mutex_lock(&memstream_list_mutex);
    for (pprev = &memstream_list; *pprev != NULL; pprev = &(*pprev)->next) {
        if ((*pprev)->fp == fp) {
            entry  = *pprev;
            *pprev = entry->next;
            pthread_mutex_unlock(&memstream_list_mutex);
            return entry;
        }
    }
    pthread_mutex_unlock(&memstream_list_mutex);
    return NULL;
}

/* Find an entry without removing it; caller must hold the list mutex. */
static struct memstream_entry *
memstream_find_locked(FILE *fp)
{
    struct memstream_entry *entry;
    for (entry = memstream_list; entry != NULL; entry = entry->next) {
        if (entry->fp == fp)
            return entry;
    }
    return NULL;
}

FILE *
open_memstream(char **ptr, size_t *sizeloc)
{
    FILE                   *fp;
    struct memstream_entry *entry;

    if (ptr == NULL || sizeloc == NULL) {
        errno = EINVAL;
        return NULL;
    }

    fp = tmpfile();
    if (fp == NULL)
        return NULL;

    entry = (struct memstream_entry *)malloc(sizeof(*entry));
    if (entry == NULL) {
        /* Use real fclose since our interposer would find no entry */
        ensure_real_fns();
        real_fclose(fp);
        return NULL;
    }

    *ptr     = NULL;
    *sizeloc = 0;

    entry->fp      = fp;
    entry->ptr     = ptr;
    entry->sizeloc = sizeloc;

    pthread_mutex_lock(&memstream_list_mutex);
    entry->next    = memstream_list;
    memstream_list = entry;
    pthread_mutex_unlock(&memstream_list_mutex);

    return fp;
}

/* =========================================================================
 * fclose / fflush interposers
 * ========================================================================= */

int
fclose(FILE *fp)
{
    struct memstream_entry *entry;

    ensure_real_fns();

    /*
     * If this FILE belongs to an open_memstream, sync *ptr/*sizeloc and
     * remove it from the registry before delegating to libc fclose.
     */
    entry = memstream_remove(fp);
    if (entry != NULL) {
        memstream_sync(fp, entry);
        free(entry);
    }

    return real_fclose(fp);
}

int
fflush(FILE *fp)
{
    struct memstream_entry *entry = NULL;

    ensure_real_fns();

    /*
     * fflush(NULL) flushes all streams — we skip memstream syncing in that
     * case because iterating all entries while calling real_fflush(NULL) on
     * the underlying tmpfiles would double-flush everything else.  The
     * caller's *ptr/*sizeloc will be updated on the next explicit
     * fflush(fp) or fclose(fp).
     */
    if (fp != NULL) {
        pthread_mutex_lock(&memstream_list_mutex);
        entry = memstream_find_locked(fp);
        pthread_mutex_unlock(&memstream_list_mutex);

        if (entry != NULL)
            memstream_sync(fp, entry);
    }

    return real_fflush(fp);
}
