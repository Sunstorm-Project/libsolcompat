/*
 * override/sys/auxv.h — Linux getauxval() stub for Solaris 7
 *
 * Linux glibc's <sys/auxv.h> exposes getauxval() for querying ELF
 * auxiliary vector entries (AT_HWCAP, AT_PLATFORM, AT_RANDOM, etc.).
 * Solaris uses a different mechanism — getisax() (in <sys/auxv.h> on
 * Solaris 11+, or just absent on older releases).
 *
 * Software written for glibc (Qt 6, libcrypto, etc.) does:
 *   #include <sys/auxv.h>
 *   unsigned long hwcap = getauxval(AT_HWCAP);
 *   if (hwcap & HWCAP_SPARC_VIS) { ... }
 *
 * Stubbing getauxval() to return 0 makes those code paths take their
 * "no extensions detected" fallback, which is always safe.
 *
 * If a future libsolcompat version wants to expose real hwcap data, it
 * can use solcompat/hwcap.h's getisax() to populate this stub.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_SYS_AUXV_H
#define _SOLCOMPAT_OVERRIDE_SYS_AUXV_H

#ifdef __sun

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Linux ELF aux-vector type constants.  We define the most commonly
 * queried ones so software like Qt that does
 *   if (getauxval(AT_HWCAP) & HWCAP_SPARC_VIS) ...
 * compiles without further patching.  All return 0 from our stub.
 */
#ifndef AT_NULL
#define AT_NULL      0
#endif
#ifndef AT_IGNORE
#define AT_IGNORE    1
#endif
#ifndef AT_EXECFD
#define AT_EXECFD    2
#endif
#ifndef AT_PHDR
#define AT_PHDR      3
#endif
#ifndef AT_PHENT
#define AT_PHENT     4
#endif
#ifndef AT_PHNUM
#define AT_PHNUM     5
#endif
#ifndef AT_PAGESZ
#define AT_PAGESZ    6
#endif
#ifndef AT_BASE
#define AT_BASE      7
#endif
#ifndef AT_FLAGS
#define AT_FLAGS     8
#endif
#ifndef AT_ENTRY
#define AT_ENTRY     9
#endif
#ifndef AT_NOTELF
#define AT_NOTELF    10
#endif
#ifndef AT_UID
#define AT_UID       11
#endif
#ifndef AT_EUID
#define AT_EUID      12
#endif
#ifndef AT_GID
#define AT_GID       13
#endif
#ifndef AT_EGID
#define AT_EGID      14
#endif
#ifndef AT_PLATFORM
#define AT_PLATFORM  15
#endif
#ifndef AT_HWCAP
#define AT_HWCAP     16
#endif
#ifndef AT_CLKTCK
#define AT_CLKTCK    17
#endif
#ifndef AT_SECURE
#define AT_SECURE    23
#endif
#ifndef AT_BASE_PLATFORM
#define AT_BASE_PLATFORM 24
#endif
#ifndef AT_RANDOM
#define AT_RANDOM    25
#endif
#ifndef AT_HWCAP2
#define AT_HWCAP2    26
#endif
#ifndef AT_EXECFN
#define AT_EXECFN    31
#endif

/*
 * getauxval — query an ELF auxiliary vector entry.
 *
 * On Linux glibc this returns the actual aux-vector value.  On Solaris
 * 7 we have no equivalent mechanism, so we return 0 — the same value
 * glibc returns when the entry is not present.  Callers should treat
 * 0 as "feature not detected" and take their fallback path.
 *
 * This stub is implemented in src/hwcap.c.
 */
extern unsigned long getauxval(unsigned long type);

#ifdef __cplusplus
}
#endif

#else /* !__sun */
#include_next <sys/auxv.h>
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_SYS_AUXV_H */
