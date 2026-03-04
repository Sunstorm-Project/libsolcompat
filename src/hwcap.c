/*
 * hwcap.c — getisax() stub for Solaris 7
 *
 * Solaris 9 introduced getisax() for querying CPU instruction-set
 * features.  On Solaris 7 (and SPARCstation-4 / microSPARC-II) there
 * are no VIS/VIS2 extensions to report, so we simply zero the array
 * and return 0.
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
