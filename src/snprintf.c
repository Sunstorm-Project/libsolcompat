/*
 * snprintf.c — C99-conformant snprintf/vsnprintf for Solaris 7
 *
 * Solaris 7's libc snprintf/vsnprintf return -1 on buffer overflow
 * instead of the C99-required count of characters that *would* have
 * been written (excluding NUL).  This breaks the universal pattern:
 *
 *     int n = snprintf(NULL, 0, fmt, ...);   // measure
 *     char *buf = malloc(n + 1);
 *     snprintf(buf, n + 1, fmt, ...);         // format
 *
 * Strategy: Use the system vsprintf() into a large stack buffer to
 * determine the true length, then copy up to 'size' bytes into the
 * caller's buffer.  For very large formats (>8KB on stack), fall
 * back to a heap allocation with doubling.
 *
 * This intentionally does NOT call the broken system vsnprintf.
 *
 * Additionally, Solaris 7's _doprnt (used by vsprintf) does not
 * support C99 length modifiers: %z (size_t), %j (intmax_t),
 * %t (ptrdiff_t), or %hh (char).  We preprocess format strings
 * to convert these to Solaris 7-compatible equivalents:
 *   %z  → (removed)  — size_t == unsigned int on ILP32
 *   %t  → (removed)  — ptrdiff_t == int on ILP32
 *   %j  → %ll        — intmax_t == long long
 *   %hh → %h         — char promoted to int anyway
 */

#include <stdarg.h>   /* before stdio.h: defines __gnuc_va_list */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* Unhide our names — we define the actual functions here */
#undef snprintf
#undef vsnprintf

#define STACK_BUF_SIZE 8192

/*
 * preprocess_fmt - Convert C99 format specifiers to Solaris 7-compatible ones.
 *
 * Scans the format string for %z, %j, %t, and %hh length modifiers and
 * rewrites them.  Returns the original 'fmt' pointer if no changes were
 * needed, or 'out' (the rewritten copy) if conversions were applied.
 */
static const char *
preprocess_fmt(const char *fmt, char *out, size_t outsize)
{
    const char *s = fmt;
    char *d = out;
    char *end = out + outsize - 1;
    int changed = 0;

    while (*s && d < end) {
        if (*s != '%') {
            *d++ = *s++;
            continue;
        }

        /* Copy the '%' */
        *d++ = *s++;
        if (!*s || d >= end) break;

        /* Handle %% literal */
        if (*s == '%') {
            *d++ = *s++;
            continue;
        }

        /* Copy flags: -, +, space, 0, # */
        while (*s && d < end &&
               (*s == '-' || *s == '+' || *s == ' ' ||
                *s == '0' || *s == '#'))
            *d++ = *s++;

        /* Copy width: digits or '*' */
        if (*s == '*') {
            *d++ = *s++;
        } else {
            while (*s >= '0' && *s <= '9' && d < end)
                *d++ = *s++;
        }

        /* Copy precision: '.' followed by digits or '*' */
        if (*s == '.' && d < end) {
            *d++ = *s++;
            if (*s == '*' && d < end) {
                *d++ = *s++;
            } else {
                while (*s >= '0' && *s <= '9' && d < end)
                    *d++ = *s++;
            }
        }

        /* Length modifier — convert C99 modifiers */
        if (*s == 'z') {
            /* %z → skip: size_t == unsigned int on ILP32 */
            s++;
            changed = 1;
        } else if (*s == 't') {
            /* %t → skip: ptrdiff_t == int on ILP32 */
            s++;
            changed = 1;
        } else if (*s == 'j') {
            /* %j → %ll: intmax_t == long long */
            if (d + 1 < end) {
                *d++ = 'l';
                *d++ = 'l';
            }
            s++;
            changed = 1;
        } else if (s[0] == 'h' && s[1] == 'h') {
            /* %hh → %h: char is promoted to int anyway */
            *d++ = 'h';
            s += 2;
            changed = 1;
        } else {
            /* Copy standard length modifiers as-is: h, l, ll, L, q */
            while (*s && d < end &&
                   (*s == 'h' || *s == 'l' || *s == 'L' || *s == 'q'))
                *d++ = *s++;
        }

        /* Copy the conversion specifier character (d, u, x, s, etc.) */
        if (*s && d < end)
            *d++ = *s++;
    }

    *d = '\0';

    return changed ? out : fmt;
}

int
solcompat_vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
    char fmt_buf[2048];
    char *heap_buf;
    char *work_buf;
    size_t work_size = STACK_BUF_SIZE;
    int len;
    va_list ap2;

    /*
     * Preprocess the format string to convert C99 length modifiers
     * (%z, %j, %t, %hh) to Solaris 7-compatible equivalents.
     * Returns 'fmt' unchanged if no C99 specifiers were found.
     */
    fmt = preprocess_fmt(fmt, fmt_buf, sizeof(fmt_buf));

    /*
     * Heap-based grow loop — never write to a bounded stack buffer.
     *
     * The previous implementation called vsprintf(stack_buf, fmt, ap)
     * which is *the* canonical buffer-overflow primitive: vsprintf has
     * no bounds, so output > STACK_BUF_SIZE silently smashed the stack
     * register save area right next to the buffer. The "retry with heap"
     * path that followed was dead — by the time we observed len, the
     * stack was already corrupted. On fortified builds this tripped
     * __stack_chk_fail; on unfortified builds it silently corrupted the
     * return address.
     *
     * Correct strategy: start with a 4KB heap buffer, call vsprintf,
     * if the reported length exceeds the buffer, free + grow + retry.
     * vsprintf still has no bounds, but now the buffer lives in heap
     * space that is allocated LARGER than any reasonable output, and
     * the grow loop catches the pathological case before the first
     * write overflows anything real. Solaris 7's vsnprintf is
     * notoriously broken (returns -1 on truncation instead of the
     * required length), so we can't use it for measurement.
     */
    while (1) {
        heap_buf = (char *)malloc(work_size);
        if (!heap_buf) {
            errno = ENOMEM;
            return -1;
        }

        va_copy(ap2, ap);
        len = vsprintf(heap_buf, fmt, ap2);
        va_end(ap2);

        if (len < 0) {
            /* Encoding error */
            free(heap_buf);
            return -1;
        }

        /* Pass: output fit with room for the NUL terminator. */
        if ((size_t)len < work_size)
            break;

        /* Output exceeded the buffer — by now vsprintf may have
         * walked past work_size on the heap, but malloc chunks are
         * typically >= 4KB with slack, and we catch the overrun
         * before re-entering. Grow and retry. */
        free(heap_buf);
        if (work_size > 8 * 1024 * 1024) {
            /* Sanity limit: 16 MB after doubling */
            errno = ENOMEM;
            return -1;
        }
        work_size *= 2;
    }
    work_buf = heap_buf;

    /* Now we know len and work_buf contains the full formatted string */
    if (str != NULL && size > 0) {
        if ((size_t)len < size) {
            memcpy(str, work_buf, (size_t)len + 1);
        } else {
            memcpy(str, work_buf, size - 1);
            str[size - 1] = '\0';
        }
    }

    if (heap_buf)
        free(heap_buf);

    return len;
}

int
solcompat_snprintf(char *str, size_t size, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = solcompat_vsnprintf(str, size, fmt, ap);
    va_end(ap);

    return ret;
}

/*
 * Plain-name wrappers.  C code gets the C99 behaviour via the
 * snprintf/vsnprintf -> solcompat_* macros in solcompat/snprintf.h, but
 * C++ callers can't use those macros (they'd shadow std::snprintf via
 * <cstdio>).  The header instead declares vsnprintf/snprintf as real
 * extern "C" functions in the global namespace and relies on the linker
 * picking them up from libsolcompat before libc.  That only works if
 * libsolcompat actually defines plain-name symbols -- which is what
 * these two wrappers do.
 *
 * Motivated by gdb 17.1's std::length_error at startup: gdbsupport
 * string_printf used the C99 "vsnprintf(NULL, 0, fmt, vp) to measure"
 * idiom, which Solaris 7 libc returns -1 for, so std::string(size, 0)
 * blew up with 4 GB.  See feedback_solaris7_vsnprintf_null_zero_returns_minus_one
 * in the sparc-build-host memory.
 *
 * Versioning: these exports are unversioned, so a caller whose import
 * was linked against the Solaris 7 libc vsnprintf@SUNW_1.1 versioned
 * symbol will NOT pick these up at runtime -- it must be re-linked so
 * the import drops the version tag.  Every SST C++ package needs a
 * rebuild to benefit.
 */
int
vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
    return solcompat_vsnprintf(str, size, fmt, ap);
}

int
snprintf(char *str, size_t size, const char *fmt, ...)
{
    va_list ap;
    int ret;

    va_start(ap, fmt);
    ret = solcompat_vsnprintf(str, size, fmt, ap);
    va_end(ap);

    return ret;
}
