/*
 * solcompat/hwcap.h — Hardware capability stubs for Solaris 7
 *
 * Solaris 9 introduced getisax() for querying CPU instruction-set
 * extensions (VIS, VIS2, etc.).  Solaris 7 doesn't have this API.
 *
 * GCC's libgcc/config/sparc/sol2-unwind.h calls getisax() to decide
 * unwinding strategy.  Our stub returns 0 (no extensions detected),
 * which causes a safe fallback to the generic unwind path.
 */
#ifndef SOLCOMPAT_HWCAP_H
#define SOLCOMPAT_HWCAP_H

#ifdef __sun

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * getisax — Solaris instruction set architecture extensions query.
 *
 * On Solaris 9+, fills array with bitmasks of supported ISA extensions.
 * On Solaris 7, we report no extensions (return 0, clear array).
 *
 * Parameters:
 *   array - output buffer for ISA extension bitmasks
 *   n     - number of uint32_t elements in array
 *
 * Returns: number of array elements filled (0 on Solaris 7 stub)
 */
int getisax(uint32_t *array, int n);

#ifdef __cplusplus
}
#endif

#endif /* __sun */

#endif /* SOLCOMPAT_HWCAP_H */
