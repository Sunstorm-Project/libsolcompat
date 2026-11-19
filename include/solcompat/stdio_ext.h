/*
 * solcompat/stdio_ext.h — Missing stdio functions
 *
 * vasprintf, asprintf, dprintf, getline, getdelim, fmemopen, open_memstream
 */
#ifndef SOLCOMPAT_STDIO_EXT_H
#define SOLCOMPAT_STDIO_EXT_H

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

int vasprintf(char **strp, const char *fmt, va_list ap);

int asprintf(char **strp, const char *fmt, ...);

int dprintf(int fd, const char *fmt, ...);
int vdprintf(int fd, const char *fmt, va_list ap);

ssize_t getline(char **lineptr, size_t *n, FILE *stream);

ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);

FILE *fmemopen(void *buf, size_t size, const char *mode);

FILE *open_memstream(char **ptr, size_t *sizeloc);

/*
 * fclose and fflush are interposed by memstream.c to support open_memstream.
 * The standard declarations from <stdio.h> already cover them; no extra
 * declarations are needed here.
 */

/* --- preadv / pwritev (scatter/gather I/O at offset) --- */
#include <sys/uio.h>
ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset);
ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_STDIO_EXT_H */
