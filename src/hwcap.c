/*
 * hwcap.c — Hardware capability stubs for Solaris 7
 *
 * Provides:
 *   getisax()    — Solaris 9+ ISA extension query (returns 0/empty)
 *   getauxval()  — Linux glibc ELF auxiliary vector query (returns 0)
 *
 * Both stubs return "no information" so callers take their fallback
 * paths.  SPARCstation-4 / microSPARC-II has no VIS/VIS2 extensions
 * to report, and Solaris 7 has no equivalent of the Linux aux-vector,
 * so this is the most accurate answer we can give without lying.
 */

#include <sys/types.h>
#include <string.h>

int
getisax(uint32_t *array, int n)
{
    if (array && n > 0) {
        memset(array, 0, (size_t)n * sizeof(uint32_t));
    }
    return 0;
}

/*
 * getauxval — Linux glibc API for querying ELF aux-vector entries.
 *
 * Solaris 7 has no equivalent.  We return 0 for any query, which is
 * the same value glibc returns when the aux-vector entry is absent.
 * Callers (Qt 6, libcrypto, etc.) treat 0 as "feature not detected"
 * and take their fallback path.
 *
 * If a future libsolcompat version wants to expose real hwcap data
 * for AT_HWCAP queries, it can call into the existing getisax()
 * shim above.  For now we return 0 universally.
 */
unsigned long
getauxval(unsigned long type)
{
    (void)type;
    return 0UL;
}
