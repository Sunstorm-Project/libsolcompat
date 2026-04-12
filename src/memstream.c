/*
 * memstream.c — fmemopen and open_memstream for Solaris 7
 *
 * fmemopen("r"):
 *   Opens /dev/null to obtain a real fd, fdopen()'s it for a properly
 *   initialised Solaris FILE slot, then redirects _cnt/_ptr/_base to the
 *   caller's read-only memory.  The pre-loaded _cnt bytes are served
 *   directly; when exhausted, read() on /dev/null returns 0 (EOF).
 *
 * fmemopen("w") / fmemopen("a"):
 *   Backed by a tmpfile() registered in a process-wide table.  fclose()
 *   and fflush() are interposed: on close the written bytes are copied
 *   from the tmpfile back into the caller's fixed buffer.
 *
 * open_memstream:
 *   Same tmpfile + registry mechanism as fmemopen-write, but the buffer
 *   grows dynamically via malloc and the caller receives it via *ptr /
 *   *sizeloc.
 *
 * Threading:
 *   The registry is protected by a mutex.  fclose() on the same FILE*
 *   from two threads concurrently is undefined behaviour (same as libc).
 *
 * Link requirement:
 *   fclose/fflush interposition uses dlsym(RTLD_NEXT).  Callers must
 *   link with -ldl.
 */

#include <stdarg.h>   /* __gnuc_va_list used by Solaris 7 stdio.h prototypes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>

/* Solaris 7 FILE struct internals (32-bit) */
#include <stdio_impl.h>

#ifndef _IOMYBUF
#  define _IOMYBUF  0010
#endif
#ifndef _NFILE
#  define _NFILE    20
#endif

extern unsigned char *_bufendtab[];

/* Pointers to the real libc fclose/fflush, resolved once via RTLD_NEXT */
static int (*real_fclose)(FILE *) = NULL;
static int (*real_fflush)(FILE *) = NULL;

static void
ensure_real_fns(void)
{
    if (real_fclose == NULL)
        real_fclose = (int (*)(FILE *))dlsym(RTLD_NEXT, "fclose");
    if (real_fflush == NULL)
        real_fflush = (int (*)(FILE *))dlsym(RTLD_NEXT, "fflush");
}

/* =========================================================================
 * Registry of active tmpfile-backed streams
 * ========================================================================= */

/*
 * If fixed_buf != NULL this is a fmemopen-write entry: on close the
 * written bytes are copied into fixed_buf (up to fixed_size bytes) and
 * NUL-terminated.
 *
 * If ptr != NULL this is an open_memstream entry: *ptr is updated to a
 * freshly malloc'd buffer containing all written bytes, *sizeloc is set
 * to the byte count.
 */
struct memstream_entry {
    FILE                   *fp;
    /* open_memstream fields */
    char                  **ptr;
    size_t                 *sizeloc;
    /* fmemopen-write fields */
    void                   *fixed_buf;
    size_t                  fixed_size;
    /* fmemopen-read: both ptr and fixed_buf are NULL.
     * We still register these so fclose can clean up _bufendtab. */
    struct memstream_entry *next;
};

static struct memstream_entry *memstream_list       = NULL;
static pthread_mutex_t         memstream_list_mutex = PTHREAD_MUTEX_INITIALIZER;

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

/*
 * Push any stdio-buffered data into the tmpfile, then sync the entry's
 * output buffer from the tmpfile contents.  Leaves fp positioned at EOF
 * so subsequent writes append correctly.
 */
static void
memstream_sync(FILE *fp, struct memstream_entry *entry)
{
    long   byte_count;
    size_t copy_bytes;

    ensure_real_fns();
    real_fflush(fp);

    byte_count = ftell(fp);
    if (byte_count < 0)
        return;

    rewind(fp);

    if (entry->fixed_buf != NULL) {
        /* fmemopen-write: copy back into the caller's fixed buffer */
        copy_bytes = (size_t)byte_count;
        if (copy_bytes > entry->fixed_size)
            copy_bytes = entry->fixed_size;
        fread(entry->fixed_buf, 1, copy_bytes, fp);
        /* NUL-terminate if there is room */
        if (copy_bytes < entry->fixed_size)
            ((char *)entry->fixed_buf)[copy_bytes] = '\0';
    } else {
        /* open_memstream: update *ptr / *sizeloc with a fresh allocation */
        char *new_buf = (char *)malloc((size_t)byte_count + 1);
        if (new_buf == NULL) {
            fseek(fp, byte_count, SEEK_SET);
            return;
        }
        fread(new_buf, 1, (size_t)byte_count, fp);
        new_buf[byte_count] = '\0';
        free(*entry->ptr);
        *entry->ptr     = new_buf;
        *entry->sizeloc = (size_t)byte_count;
    }

    /* Restore write position to end so further writes append */
    fseek(fp, byte_count, SEEK_SET);
}

static struct memstream_entry *
make_entry(FILE *fp, char **ptr, size_t *sizeloc,
           void *fixed_buf, size_t fixed_size)
{
    struct memstream_entry *entry;

    entry = (struct memstream_entry *)malloc(sizeof(*entry));
    if (entry == NULL)
        return NULL;

    entry->fp         = fp;
    entry->ptr        = ptr;
    entry->sizeloc    = sizeloc;
    entry->fixed_buf  = fixed_buf;
    entry->fixed_size = fixed_size;

    pthread_mutex_lock(&memstream_list_mutex);
    entry->next    = memstream_list;
    memstream_list = entry;
    pthread_mutex_unlock(&memstream_list_mutex);

    return entry;
}

/* =========================================================================
 * fmemopen
 * ========================================================================= */

FILE *
fmemopen(void *buf, size_t size, const char *mode)
{
    int devnull_fd;
    FILE *fp;

    if (buf == NULL || size == 0 || mode == NULL) {
        errno = EINVAL;
        return NULL;
    }

    if (mode[0] == 'r') {
        /*
         * Read mode: struct manipulation — pre-fill the stdio buffer with
         * the caller's data.  /dev/null provides the backing fd; reads
         * return EOF once the pre-loaded bytes are exhausted.
         */
        devnull_fd = open("/dev/null", O_RDONLY);
        if (devnull_fd < 0)
            return NULL;

        fp = fdopen(devnull_fd, "r");
        if (fp == NULL) {
            close(devnull_fd);
            return NULL;
        }

        fp->_base = (unsigned char *)buf;
        fp->_ptr  = (unsigned char *)buf;
        fp->_cnt  = (ssize_t)size;
        fp->_flag = (unsigned char)(fp->_flag & ~_IOMYBUF);

        if (devnull_fd < _NFILE)
            _bufendtab[devnull_fd] = (unsigned char *)buf + size;

        /* Register so fclose can clean up _bufendtab.
         * Read-mode entries have both ptr==NULL and fixed_buf==NULL. */
        if (make_entry(fp, NULL, NULL, NULL, 0) == NULL) {
            ensure_real_fns();
            real_fclose(fp);
            return NULL;
        }

        return fp;
    }

    if (mode[0] == 'w' || mode[0] == 'a') {
        /*
         * Write/append mode: tmpfile-backed.  Bytes accumulate in the
         * tmpfile; on fclose the contents are copied back into buf.
         */
        fp = tmpfile();
        if (fp == NULL)
            return NULL;

        if (make_entry(fp, NULL, NULL, buf, size) == NULL) {
            ensure_real_fns();
            real_fclose(fp);
            return NULL;
        }

        /* For write mode clear the buffer; for append mode leave it */
        if (mode[0] == 'w')
            memset(buf, 0, size);

        return fp;
    }

    errno = EINVAL;
    return NULL;
}

/* =========================================================================
 * open_memstream
 * ========================================================================= */

FILE *
open_memstream(char **ptr, size_t *sizeloc)
{
    FILE *fp;

    if (ptr == NULL || sizeloc == NULL) {
        errno = EINVAL;
        return NULL;
    }

    fp = tmpfile();
    if (fp == NULL)
        return NULL;

    *ptr     = NULL;
    *sizeloc = 0;

    if (make_entry(fp, ptr, sizeloc, NULL, 0) == NULL) {
        ensure_real_fns();
        real_fclose(fp);
        return NULL;
    }

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

    entry = memstream_remove(fp);
    if (entry != NULL) {
        if (entry->fixed_buf == NULL && entry->ptr == NULL) {
            /* Read-mode fmemopen: clean up _bufendtab so the fd slot
             * doesn't carry a stale buffer-end pointer after reuse. */
            int stream_fd = fileno(fp);
            if (stream_fd >= 0 && stream_fd < _NFILE)
                _bufendtab[stream_fd] = NULL;
        } else {
            memstream_sync(fp, entry);
        }
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
     * fflush(NULL) flushes all streams.  Skip memstream syncing to avoid
     * interfering with the global flush; *ptr/*sizeloc will be updated on
     * the next explicit fflush(fp) or fclose(fp).
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
