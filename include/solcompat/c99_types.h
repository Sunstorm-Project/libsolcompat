/*
 * solcompat/c99_types.h -- C99 integer type compatibility
 *
 * Solaris 7 has <inttypes.h> with int8_t..int64_t but lacks both
 * <stdint.h> (C99) and the PRIxxx/SCNxxx format macros.
 *
 * The override/inttypes.h header now provides both PRIxxx and SCNxxx
 * macros automatically.  This internal header is kept for backward
 * compatibility and adds SCNxxx macros if included directly.
 */
#ifndef SOLCOMPAT_C99_TYPES_H
#define SOLCOMPAT_C99_TYPES_H

#include <inttypes.h>
#include <limits.h>

/* Scan format macros — Solaris 7 inttypes.h has PRI* but not SCN* */
#ifndef SCNd8
#define SCNd8   "hhd"
#endif
#ifndef SCNd16
#define SCNd16  "hd"
#endif
#ifndef SCNd32
#define SCNd32  "d"
#endif
#ifndef SCNd64
#define SCNd64  "lld"
#endif

#ifndef SCNi8
#define SCNi8   "hhi"
#endif
#ifndef SCNi16
#define SCNi16  "hi"
#endif
#ifndef SCNi32
#define SCNi32  "i"
#endif
#ifndef SCNi64
#define SCNi64  "lli"
#endif

#ifndef SCNu8
#define SCNu8   "hhu"
#endif
#ifndef SCNu16
#define SCNu16  "hu"
#endif
#ifndef SCNu32
#define SCNu32  "u"
#endif
#ifndef SCNu64
#define SCNu64  "llu"
#endif

#ifndef SCNx8
#define SCNx8   "hhx"
#endif
#ifndef SCNx16
#define SCNx16  "hx"
#endif
#ifndef SCNx32
#define SCNx32  "x"
#endif
#ifndef SCNx64
#define SCNx64  "llx"
#endif

/* intmax_t format macros */
#ifndef SCNdMAX
#define SCNdMAX "lld"
#endif
#ifndef SCNiMAX
#define SCNiMAX "lli"
#endif
#ifndef SCNuMAX
#define SCNuMAX "llu"
#endif
#ifndef SCNxMAX
#define SCNxMAX "llx"
#endif

/* intmax constant macros */
#ifndef INTMAX_C
#define INTMAX_C(c)  c ## LL
#endif
#ifndef UINTMAX_C
#define UINTMAX_C(c) c ## ULL
#endif

#endif /* SOLCOMPAT_C99_TYPES_H */
