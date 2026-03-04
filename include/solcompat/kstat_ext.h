/*
 * solcompat/kstat_ext.h — kstat extensions for Solaris 7
 *
 * Solaris 7's <sys/kstat.h> lacks features added in Solaris 8+:
 *   - KSTAT_DATA_STRING (data type 9)
 *   - KSTAT_NAMED_STR_PTR / KSTAT_NAMED_STR_BUFLEN macros
 *   - kstat_named_t.value.str member
 *
 * Since the Solaris 7 kstat_named_t struct doesn't have the `str`
 * member at all, code that uses KSTAT_DATA_STRING simply cannot work.
 * We provide the type constant and macros as no-ops / stubs so that
 * code compiles, but runtime access is guarded by the type check.
 *
 * Typical usage in GCC's driver-sparc.c:
 *   if (brand->data_type == KSTAT_DATA_STRING)
 *       cpu_brand = KSTAT_NAMED_STR_PTR(brand);
 *
 * Since no Solaris 7 kstat will ever have data_type == 9, the branch
 * is dead code and KSTAT_NAMED_STR_PTR is never actually evaluated.
 */
#ifndef SOLCOMPAT_KSTAT_EXT_H
#define SOLCOMPAT_KSTAT_EXT_H

#ifdef __sun

#include <kstat.h>

/*
 * KSTAT_DATA_STRING — named kstat string data type.
 * Added in Solaris 8; value is 9 in all later releases.
 */
#ifndef KSTAT_DATA_STRING
#define KSTAT_DATA_STRING  9
#endif

/*
 * KSTAT_NAMED_STR_PTR — Get pointer to string from a named kstat.
 * On Solaris 8+, this accesses kstat_named_t.value.str.addr.str.
 * On Solaris 7 this is dead code (no kstat ever has type STRING),
 * so we return a safe null pointer.
 */
#ifndef KSTAT_NAMED_STR_PTR
#define KSTAT_NAMED_STR_PTR(knptr) ((char *)0)
#endif

/*
 * KSTAT_NAMED_STR_BUFLEN — Get buffer length of a named kstat string.
 * Same situation as KSTAT_NAMED_STR_PTR — dead code on Solaris 7.
 */
#ifndef KSTAT_NAMED_STR_BUFLEN
#define KSTAT_NAMED_STR_BUFLEN(knptr) (0)
#endif

#endif /* __sun */

#endif /* SOLCOMPAT_KSTAT_EXT_H */
