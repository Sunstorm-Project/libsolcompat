/*
 * stdio.c — Missing stdio functions for Solaris 7
 *
 * vasprintf, asprintf, dprintf, getline, getdelim
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

/* We need our fixed snprintf */
extern int solcompat_vsnprintf(char *, size_t, const char *, va_list);

int
vasprintf(char **strp, const char *fmt, va_list ap)
{
    va_list ap2;
    int len;
    char *buf;

    /* Measure required length */
    va_copy(ap2, ap);
    len = solcompat_vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);

    if (len < 0) {
        *strp = NULL;
        return -1;
    }

    buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        *strp = NULL;
        errno = ENOMEM;
        return -1;
    }

    va_copy(ap2, ap);
    solcompat_vsnprintf(buf, (size_t)len + 1, fmt, ap2);
    va_end(ap2);

    *strp = buf;
    return len;
}

int
asprintf(char **strp, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = vasprintf(strp, fmt, ap);
    va_end(ap);

    return ret;
}

int
dprintf(int fd, const char *fmt, ...)
{
    va_list ap;
    char *buf;
    int len;
    ssize_t written, total;

    va_start(ap, fmt);
    len = vasprintf(&buf, fmt, ap);
    va_end(ap);

    if (len < 0)
        return -1;

    total = 0;
    while (total < len) {
        written = write(fd, buf + total, (size_t)(len - total));
        if (written < 0) {
            if (errno == EINTR)
                continue;
            free(buf);
            return -1;
        }
        total += written;
    }

    free(buf);
    return len;
}

#define GETDELIM_INITIAL_SIZE 128

ssize_t
getdelim(char **lineptr, size_t *n, int delim, FILE *stream)
{
    char *buf;
    size_t bufsize;
    size_t pos = 0;
    int c;

    if (!lineptr || !n || !stream) {
        errno = EINVAL;
        return -1;
    }

    if (*lineptr == NULL || *n == 0) {
        bufsize = GETDELIM_INITIAL_SIZE;
        buf = (char *)malloc(bufsize);
        if (!buf) {
            errno = ENOMEM;
            return -1;
        }
        *lineptr = buf;
        *n = bufsize;
    } else {
        buf = *lineptr;
        bufsize = *n;
    }

    while ((c = fgetc(stream)) != EOF) {
        if (pos + 1 >= bufsize) {
            bufsize *= 2;
            buf = (char *)realloc(buf, bufsize);
            if (!buf) {
                errno = ENOMEM;
                return -1;
            }
            *lineptr = buf;
            *n = bufsize;
        }
        buf[pos++] = (char)c;
        if (c == delim)
            break;
    }

    if (pos == 0 && c == EOF)
        return -1;

    buf[pos] = '\0';
    return (ssize_t)pos;
}

ssize_t
getline(char **lineptr, size_t *n, FILE *stream)
{
    return getdelim(lineptr, n, '\n', stream);
}

/*
 * preadv / pwritev — scatter/gather I/O at an offset.
 *
 * Solaris 7 has pread/pwrite (POSIX.1) but not preadv/pwritev (POSIX.1-2008).
 * We emulate by looping over the iovec array and calling pread/pwrite for
 * each element, advancing the file offset manually.
 *
 * This is not atomic (a concurrent writer can interleave), but neither is
 * the kernel implementation on most systems for regular files.
 */
#include <sys/uio.h>

ssize_t
preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
    ssize_t total_bytes = 0;
    int vector_index;

    for (vector_index = 0; vector_index < iovcnt; vector_index++) {
        ssize_t bytes_read;

        if (iov[vector_index].iov_len == 0)
            continue;

        bytes_read = pread(fd, iov[vector_index].iov_base,
                           iov[vector_index].iov_len, offset);
        if (bytes_read < 0)
            return (total_bytes > 0) ? total_bytes : -1;
        if (bytes_read == 0)
            break;

        total_bytes += bytes_read;
        offset += bytes_read;

        /* Short read means no more data available right now */
        if ((size_t)bytes_read < iov[vector_index].iov_len)
            break;
    }

    return total_bytes;
}

ssize_t
pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
    ssize_t total_bytes = 0;
    int vector_index;

    for (vector_index = 0; vector_index < iovcnt; vector_index++) {
        ssize_t bytes_written;

        if (iov[vector_index].iov_len == 0)
            continue;

        bytes_written = pwrite(fd, iov[vector_index].iov_base,
                               iov[vector_index].iov_len, offset);
        if (bytes_written < 0)
            return (total_bytes > 0) ? total_bytes : -1;

        total_bytes += bytes_written;
        offset += bytes_written;

        /* Short write means we should stop */
        if ((size_t)bytes_written < iov[vector_index].iov_len)
            break;
    }

    return total_bytes;
}
