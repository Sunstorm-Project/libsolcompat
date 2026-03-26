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

#ifndef HAVE_VASPRINTF
int vasprintf(char **strp, const char *fmt, va_list ap);
#endif

#ifndef HAVE_ASPRINTF
int asprintf(char **strp, const char *fmt, ...);
#endif

#ifndef HAVE_DPRINTF
int dprintf(int fd, const char *fmt, ...);
#endif

#ifndef HAVE_GETLINE
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif

#ifndef HAVE_GETDELIM
ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);
#endif

#ifndef HAVE_FMEMOPEN
FILE *fmemopen(void *buf, size_t size, const char *mode);
#endif

#ifndef HAVE_OPEN_MEMSTREAM
FILE *open_memstream(char **ptr, size_t *sizeloc);
#endif

/*
 * fclose and fflush are interposed by memstream.c to support open_memstream.
 * The standard declarations from <stdio.h> already cover them; no extra
 * declarations are needed here.
 */

/* --- preadv / pwritev (scatter/gather I/O at offset) --- */
#include <sys/uio.h>
#ifndef HAVE_PREADV
ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset);
#endif
#ifndef HAVE_PWRITEV
ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SOLCOMPAT_STDIO_EXT_H */
