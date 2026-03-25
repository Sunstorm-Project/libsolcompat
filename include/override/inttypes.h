/*
 * override/inttypes.h — C99 integer types + format macros for Solaris 7
 *
 * Solaris 7's <inttypes.h> guards its type definitions behind
 *   #if __STDC__ - 0 == 0
 * which is false under GCC (where __STDC__ == 1), so it provides NO
 * integer types in standard C mode.  This override pulls in our
 * override/stdint.h first to ensure int8_t..int64_t and friends are
 * always defined, then chains to the system header for PRI.../SCN... macros.
 *
 * Part of libsolcompat - https://github.com/Sunstorm-Project/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_INTTYPES_H
#define _SOLCOMPAT_OVERRIDE_INTTYPES_H

/* Ensure all C99 integer types are defined before the system header runs. */
#include <stdint.h>

/* Pull in the real Solaris 7 /usr/include/inttypes.h */
#include_next <inttypes.h>

/* ================================================================
 * Printf format macros — signed
 * ================================================================ */
#ifndef PRId8
#define PRId8   "d"
#endif
#ifndef PRId16
#define PRId16  "d"
#endif
#ifndef PRId32
#define PRId32  "d"
#endif
#ifndef PRId64
#define PRId64  "lld"
#endif

#ifndef PRIi8
#define PRIi8   "i"
#endif
#ifndef PRIi16
#define PRIi16  "i"
#endif
#ifndef PRIi32
#define PRIi32  "i"
#endif
#ifndef PRIi64
#define PRIi64  "lli"
#endif

/* ================================================================
 * Printf format macros — unsigned decimal
 * ================================================================ */
#ifndef PRIu8
#define PRIu8   "u"
#endif
#ifndef PRIu16
#define PRIu16  "u"
#endif
#ifndef PRIu32
#define PRIu32  "u"
#endif
#ifndef PRIu64
#define PRIu64  "llu"
#endif

/* ================================================================
 * Printf format macros — unsigned hex
 * ================================================================ */
#ifndef PRIx8
#define PRIx8   "x"
#endif
#ifndef PRIx16
#define PRIx16  "x"
#endif
#ifndef PRIx32
#define PRIx32  "x"
#endif
#ifndef PRIx64
#define PRIx64  "llx"
#endif

#ifndef PRIX8
#define PRIX8   "X"
#endif
#ifndef PRIX16
#define PRIX16  "X"
#endif
#ifndef PRIX32
#define PRIX32  "X"
#endif
#ifndef PRIX64
#define PRIX64  "llX"
#endif

/* ================================================================
 * Printf format macros — unsigned octal
 * ================================================================ */
#ifndef PRIo8
#define PRIo8   "o"
#endif
#ifndef PRIo16
#define PRIo16  "o"
#endif
#ifndef PRIo32
#define PRIo32  "o"
#endif
#ifndef PRIo64
#define PRIo64  "llo"
#endif

/* ================================================================
 * Printf format macros — least-width types
 * ================================================================ */
#ifndef PRIdLEAST8
#define PRIdLEAST8   PRId8
#endif
#ifndef PRIdLEAST16
#define PRIdLEAST16  PRId16
#endif
#ifndef PRIdLEAST32
#define PRIdLEAST32  PRId32
#endif
#ifndef PRIdLEAST64
#define PRIdLEAST64  PRId64
#endif
#ifndef PRIuLEAST8
#define PRIuLEAST8   PRIu8
#endif
#ifndef PRIuLEAST16
#define PRIuLEAST16  PRIu16
#endif
#ifndef PRIuLEAST32
#define PRIuLEAST32  PRIu32
#endif
#ifndef PRIuLEAST64
#define PRIuLEAST64  PRIu64
#endif

/* ================================================================
 * Printf format macros — fast types (32-bit on SPARC)
 * ================================================================ */
#ifndef PRIdFAST8
#define PRIdFAST8   PRId32
#endif
#ifndef PRIdFAST16
#define PRIdFAST16  PRId32
#endif
#ifndef PRIdFAST32
#define PRIdFAST32  PRId32
#endif
#ifndef PRIdFAST64
#define PRIdFAST64  PRId64
#endif
#ifndef PRIuFAST8
#define PRIuFAST8   PRIu32
#endif
#ifndef PRIuFAST16
#define PRIuFAST16  PRIu32
#endif
#ifndef PRIuFAST32
#define PRIuFAST32  PRIu32
#endif
#ifndef PRIuFAST64
#define PRIuFAST64  PRIu64
#endif

/* ================================================================
 * Printf format macros — pointer and max-width types
 * ================================================================ */
#ifndef PRIdPTR
#define PRIdPTR  PRId32    /* ILP32 */
#endif
#ifndef PRIuPTR
#define PRIuPTR  PRIu32
#endif
#ifndef PRIxPTR
#define PRIxPTR  PRIx32
#endif

#ifndef PRIdMAX
#define PRIdMAX  PRId64
#endif
#ifndef PRIuMAX
#define PRIuMAX  PRIu64
#endif
#ifndef PRIxMAX
#define PRIxMAX  PRIx64
#endif

/* ================================================================
 * Scanf format macros — signed
 * ================================================================ */
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

/* ================================================================
 * Scanf format macros — unsigned
 * ================================================================ */
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

/* ================================================================
 * Scanf format macros — pointer and max-width types
 * ================================================================ */
#ifndef SCNdPTR
#define SCNdPTR  SCNd32
#endif
#ifndef SCNuPTR
#define SCNuPTR  SCNu32
#endif
#ifndef SCNxPTR
#define SCNxPTR  SCNx32
#endif

#ifndef SCNdMAX
#define SCNdMAX  SCNd64
#endif
#ifndef SCNiMAX
#define SCNiMAX  "lli"
#endif
#ifndef SCNuMAX
#define SCNuMAX  SCNu64
#endif
#ifndef SCNxMAX
#define SCNxMAX  SCNx64
#endif

/* ================================================================
 * imaxabs / imaxdiv — C99 intmax_t arithmetic
 * ================================================================ */
#ifndef _SOLCOMPAT_IMAXDIV_DEFINED
#define _SOLCOMPAT_IMAXDIV_DEFINED
typedef struct {
    long long quot;
    long long rem;
} imaxdiv_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

long long imaxabs(long long j);
imaxdiv_t imaxdiv(long long numer, long long denom);
intmax_t strtoimax(const char *nptr, char **endptr, int base);
uintmax_t strtoumax(const char *nptr, char **endptr, int base);

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_OVERRIDE_INTTYPES_H */
