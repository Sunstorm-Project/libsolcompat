/*
 * stdlib.c — Missing stdlib functions for Solaris 7
 *
 * setenv, unsetenv, mkdtemp
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>                  /* time() in mkdtemp fallback seed */
#include "solcompat/random.h"      /* getentropy prototype */

extern int solcompat_snprintf(char *, size_t, const char *, ...);

/*
 * _Exit — C99 immediate program termination.
 *
 * Like exit() but does NOT call atexit handlers or flush stdio buffers.
 * Solaris 7 doesn't have it; _exit() (POSIX) does the same thing.
 */
void
_Exit(int status)
{
    _exit(status);
}

int
setenv(const char *name, const char *value, int overwrite)
{
    char *envstr;
    size_t nlen, vlen;

    if (!name || !name[0] || strchr(name, '=')) {
        errno = EINVAL;
        return -1;
    }

    if (!overwrite && getenv(name))
        return 0;

    nlen = strlen(name);
    vlen = strlen(value);
    envstr = (char *)malloc(nlen + vlen + 2);
    if (!envstr) {
        errno = ENOMEM;
        return -1;
    }

    memcpy(envstr, name, nlen);
    envstr[nlen] = '=';
    memcpy(envstr + nlen + 1, value, vlen + 1);

    if (putenv(envstr) != 0) {
        free(envstr);
        return -1;
    }
    /* Note: envstr is now owned by the environment — do NOT free it */
    return 0;
}

int
unsetenv(const char *name)
{
    extern char **environ;
    char **ep, **dp;
    size_t nlen;

    if (!name || !name[0] || strchr(name, '=')) {
        errno = EINVAL;
        return -1;
    }

    nlen = strlen(name);
    for (ep = dp = environ; *ep; ep++) {
        if (strncmp(*ep, name, nlen) == 0 && (*ep)[nlen] == '=')
            continue;   /* skip this entry */
        *dp++ = *ep;
    }
    *dp = NULL;
    return 0;
}

/*
 * Fill 6 bytes of buffer with Base-62-ish random characters from the
 * TMPNAM_ALPHABET. Used by mkdtemp's TOCTOU-free loop below.
 *
 * Entropy source: getentropy (which in turn reads /dev/urandom; falls
 * back to the weak per-process mixer if SOLCOMPAT_ALLOW_WEAK_RANDOM is
 * set). If getentropy fails, we fall back to time^pid XOR state —
 * still predictable but at least non-static between processes.
 *
 * 62^6 = 5.68e10 possible suffixes per template, which is more than
 * enough collision space for the 1000-attempt retry loop.
 */
static const char TMPNAM_ALPHABET[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

static void
mkdtemp_fill_suffix(char *suffix, int retry_count)
{
    unsigned char entropy[6];
    int i;

    if (getentropy(entropy, sizeof(entropy)) != 0) {
        /* getentropy failed (strict mode with no /dev/urandom). Fall
         * back to a non-static seed mix — still insecure but better
         * than repeating the same suffix across retries. */
        unsigned long seed = (unsigned long)time(NULL);
        seed ^= (unsigned long)getpid() * 2654435761UL;
        seed ^= (unsigned long)retry_count * 16777619UL;
        for (i = 0; i < 6; i++) {
            seed = seed * 1103515245UL + 12345UL;
            entropy[i] = (unsigned char)(seed >> 16);
        }
    }
    for (i = 0; i < 6; i++)
        suffix[i] = TMPNAM_ALPHABET[entropy[i] % 62];
}

char *
mkdtemp(char *tmpl)
{
    size_t len;
    int attempt;
    char *suffix;

    if (!tmpl) {
        errno = EINVAL;
        return NULL;
    }

    len = strlen(tmpl);
    if (len < 6) {
        errno = EINVAL;
        return NULL;
    }

    suffix = tmpl + len - 6;
    if (strcmp(suffix, "XXXXXX") != 0) {
        errno = EINVAL;
        return NULL;
    }

    /*
     * Loop until we successfully mkdir with O_EXCL-equivalent semantics.
     * Previously used mktemp(3) + mkdir(), but mktemp has a well-known
     * name-disclosure race: the filename is generated, the process
     * descheduled, an attacker pre-creates the directory, mkdir fails
     * with EEXIST, retry. On Solaris 7's multi-user boxes (or shared
     * containers) that race is reachable. POSIX.1-2008 deprecated
     * mktemp precisely because of this.
     *
     * Replacement: generate 6 random chars from getentropy, try mkdir
     * directly. mkdir is atomic (O_EXCL-equivalent), so the race
     * window collapses to zero — the worst failure mode is EEXIST on
     * genuine collision, which the retry loop handles.
     */
    for (attempt = 0; attempt < 1000; attempt++) {
        mkdtemp_fill_suffix(suffix, attempt);

        if (mkdir(tmpl, 0700) == 0)
            return tmpl;

        if (errno != EEXIST) {
            /* Real error (e.g. ENOENT on parent, EACCES) */
            return NULL;
        }
        /* EEXIST — regenerate suffix and retry */
    }

    errno = EEXIST;
    return NULL;
}

/* -----------------------------------------------------------------------
 * qsort_r — reentrant sort with caller-supplied context argument.
 *
 * Implements the GNU/glibc signature:
 *   compare_fn(left_element, right_element, context_arg) → negative/0/positive
 *
 * Solaris 7 has qsort() but not qsort_r().  This implementation uses a
 * hybrid strategy: insertion sort for small subarrays (≤ INSERTION_CUTOFF
 * elements) and median-of-three Lomuto quicksort for larger ones.
 *
 * No global state is used, so the function is reentrant and thread-safe.
 * ----------------------------------------------------------------------- */

#define QSORT_INSERTION_CUTOFF 12

static void
qsort_swap_bytes(char *left_element, char *right_element, size_t byte_count)
{
    char temp_byte;
    while (byte_count--) {
        temp_byte      = *left_element;
        *left_element  = *right_element;
        *right_element = temp_byte;
        left_element++;
        right_element++;
    }
}

/*
 * Insertion sort over the closed interval [left_bound .. right_bound].
 * Used for small subarrays where the constant overhead of quicksort
 * outweighs the O(n²) cost.
 */
static void
qsort_insertion_sort(char *base,
                     size_t left_bound, size_t right_bound,
                     size_t element_size,
                     int (*compare_fn)(const void *, const void *, void *),
                     void *context_arg)
{
    size_t outer_index;
    size_t inner_index;

    for (outer_index = left_bound + 1;
         outer_index <= right_bound;
         outer_index++) {
        for (inner_index = outer_index;
             inner_index > left_bound &&
             compare_fn(base + (inner_index - 1) * element_size,
                        base + inner_index       * element_size,
                        context_arg) > 0;
             inner_index--) {
            qsort_swap_bytes(base + (inner_index - 1) * element_size,
                             base + inner_index       * element_size,
                             element_size);
        }
    }
}

/*
 * Lomuto partition over [left_bound .. right_bound].
 * The pivot must already be placed at base[right_bound] by the caller.
 * Returns the final index of the pivot after all elements ≤ pivot have
 * been moved to its left and all elements > pivot to its right.
 */
static size_t
qsort_lomuto_partition(char *base,
                       size_t left_bound, size_t right_bound,
                       size_t element_size,
                       int (*compare_fn)(const void *, const void *, void *),
                       void *context_arg)
{
    char   *pivot_element = base + right_bound * element_size;
    size_t  store_index   = left_bound;
    size_t  scan_index;

    for (scan_index = left_bound; scan_index < right_bound; scan_index++) {
        if (compare_fn(base + scan_index * element_size,
                       pivot_element, context_arg) <= 0) {
            qsort_swap_bytes(base + store_index * element_size,
                             base + scan_index  * element_size,
                             element_size);
            store_index++;
        }
    }

    /* Place pivot at its final sorted position */
    qsort_swap_bytes(base + store_index  * element_size,
                     pivot_element, element_size);
    return store_index;
}

/*
 * Recursive quicksort over the closed interval [left_bound .. right_bound].
 */
static void
qsort_recursive(char *base,
                size_t left_bound, size_t right_bound,
                size_t element_size,
                int (*compare_fn)(const void *, const void *, void *),
                void *context_arg)
{
    size_t mid_index;
    size_t pivot_index;
    size_t subarray_size;

    if (left_bound >= right_bound)
        return;

    subarray_size = right_bound - left_bound + 1;

    /* Fall back to insertion sort for small subarrays */
    if (subarray_size <= QSORT_INSERTION_CUTOFF) {
        qsort_insertion_sort(base, left_bound, right_bound,
                             element_size, compare_fn, context_arg);
        return;
    }

    /*
     * Median-of-three pivot selection.
     *
     * Sort the elements at left_bound, mid_index, and right_bound so that:
     *   base[left_bound] <= base[mid_index] <= base[right_bound]
     *
     * Then move the median to right_bound to serve as the Lomuto pivot.
     * This avoids worst-case O(n²) behaviour on sorted or reverse-sorted input.
     */
    mid_index = left_bound + (right_bound - left_bound) / 2;

    if (compare_fn(base + left_bound * element_size,
                   base + mid_index  * element_size, context_arg) > 0)
        qsort_swap_bytes(base + left_bound * element_size,
                         base + mid_index  * element_size, element_size);

    if (compare_fn(base + left_bound  * element_size,
                   base + right_bound * element_size, context_arg) > 0)
        qsort_swap_bytes(base + left_bound  * element_size,
                         base + right_bound * element_size, element_size);

    if (compare_fn(base + mid_index   * element_size,
                   base + right_bound * element_size, context_arg) > 0)
        qsort_swap_bytes(base + mid_index   * element_size,
                         base + right_bound * element_size, element_size);

    /* base[left_bound] <= base[mid_index] <= base[right_bound].
     * Swap the median to the end so Lomuto can use it as pivot.         */
    qsort_swap_bytes(base + mid_index   * element_size,
                     base + right_bound * element_size, element_size);

    pivot_index = qsort_lomuto_partition(base, left_bound, right_bound,
                                         element_size, compare_fn, context_arg);

    /* Recurse on the left partition.  Guard against underflow when
     * pivot_index == left_bound (size_t wraps on subtraction).          */
    if (pivot_index > left_bound)
        qsort_recursive(base, left_bound, pivot_index - 1,
                        element_size, compare_fn, context_arg);

    /* Recurse on the right partition */
    if (pivot_index < right_bound)
        qsort_recursive(base, pivot_index + 1, right_bound,
                        element_size, compare_fn, context_arg);
}

void
qsort_r(void *base, size_t element_count, size_t element_size,
        int (*compare_fn)(const void *, const void *, void *),
        void *context_arg)
{
    if (element_count <= 1 || element_size == 0)
        return;

    qsort_recursive((char *)base, 0, element_count - 1,
                    element_size, compare_fn, context_arg);
}

/*
 * strtof — C99 string-to-float conversion.
 * Solaris 7 libc has strtod() but not strtof().
 * This is a simple wrapper; precision is limited to what strtod provides,
 * which is fine since float is strictly narrower than double.
 */
float
strtof(const char *nptr, char **endptr)
{
    return (float)strtod(nptr, endptr);
}

/*
 * strtold — C99 string-to-long-double conversion.
 * On SPARC Solaris 7, long double is 128-bit quad precision in the ABI,
 * but GCC's soft-float implementation makes it effectively 64-bit
 * (same as double). This wrapper is sufficient.
 */
long double
strtold(const char *nptr, char **endptr)
{
    return (long double)strtod(nptr, endptr);
}

/*
 * secure_getenv — GNU extension.
 * Returns NULL if the program is running setuid/setgid.
 * Solaris 7 has no AT_SECURE auxiliary vector entry, so we just
 * check issetugid() if available, otherwise delegate to getenv().
 */
char *
secure_getenv(const char *name)
{
    /* Solaris 7 doesn't have issetugid() either, but getuid/geteuid
     * are always available. */
    if (getuid() != geteuid() || getgid() != getegid())
        return NULL;
    return getenv(name);
}
