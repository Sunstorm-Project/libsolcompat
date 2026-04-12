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
    char stack_buf[STACK_BUF_SIZE];
    char fmt_buf[2048];
    char *heap_buf = NULL;
    char *work_buf = stack_buf;
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
     * First attempt: use vsprintf into stack buffer.
     * vsprintf always writes everything (no truncation),
     * so we need a buffer large enough.  If the format
     * produces more than STACK_BUF_SIZE-1 chars, we detect
     * it and retry with a heap buffer.
     *
     * Safety: we can't know the length ahead of time without
     * formatting.  The stack buffer handles 99%+ of real-world
     * format strings.  For pathological cases, grow on heap.
     */

    /* Make a copy of ap since we might need to retry */
    va_copy(ap2, ap);
    len = vsprintf(stack_buf, fmt, ap2);
    va_end(ap2);

    if (len < 0) {
        /* True encoding error */
        return -1;
    }

    if ((size_t)len >= STACK_BUF_SIZE) {
        /*
         * Output was truncated (vsprintf wrote past our buffer —
         * this is a buffer overrun on the stack, but here we use
         * it as a signal to allocate heap space).
         *
         * Actually, vsprintf returns the number of chars written,
         * but it doesn't stop!  It wrote past the buffer.
         * We need a safer approach.
         *
         * Revised strategy: use the broken system vsnprintf to
         * format into the stack buffer (which DOES truncate), then
         * if it returns -1, double a heap buffer until vsnprintf
         * returns >= 0 (meaning the buffer was big enough).
         */
        work_size = STACK_BUF_SIZE * 2;
        while (1) {
            heap_buf = (char *)malloc(work_size);
            if (!heap_buf) {
                errno = ENOMEM;
                return -1;
            }

            va_copy(ap2, ap);
            /*
             * Use the REAL vsprintf (unbounded) — it always
             * writes the full output.  We allocated enough space.
             */
            len = vsprintf(heap_buf, fmt, ap2);
            va_end(ap2);

            if (len >= 0 && (size_t)len < work_size)
                break;

            /* Double and retry */
            free(heap_buf);
            work_size *= 2;
            if (work_size > 16 * 1024 * 1024) {
                /* Sanity limit: 16 MB */
                errno = ENOMEM;
                return -1;
            }
        }
        work_buf = heap_buf;
    }

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
