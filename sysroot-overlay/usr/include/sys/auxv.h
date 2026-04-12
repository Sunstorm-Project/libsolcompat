/*
 * Part of libsolcompat sysroot — provides Linux-style getauxval() stub for Solaris 7
 *
 * Linux glibc's <sys/auxv.h> exposes getauxval() for querying ELF
 * auxiliary vector entries.  Solaris 7 has no equivalent.  This header
 * provides AT_* constants and a getauxval() stub that returns 0 (feature
 * not detected), so software like Qt 6 and libcrypto takes its fallback
 * path.
 */
#ifndef _SOLCOMPAT_SYSROOT_SYS_AUXV_H
#define _SOLCOMPAT_SYSROOT_SYS_AUXV_H

#ifdef __cplusplus
extern "C" {
#endif

/* Linux ELF aux-vector type constants */
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
 * Returns 0 on Solaris 7 (no aux vector mechanism), causing callers
 * to take their "feature not detected" fallback path.
 */
extern unsigned long getauxval(unsigned long type);

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_SYSROOT_SYS_AUXV_H */
