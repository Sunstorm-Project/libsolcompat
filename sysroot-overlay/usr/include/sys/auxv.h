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
/* AT_UID / AT_EUID / AT_GID / AT_EGID intentionally omitted.
 * Solaris 7's <sys/vnode.h> uses these names as file-attribute bit
 * masks (0x0004, 0x0008, etc.).  Kernel-adjacent code that pulls
 * <sys/vnode.h> would collide with our auxv-vector definitions.
 * getauxval() callers almost exclusively use AT_HWCAP, AT_PLATFORM,
 * AT_PAGESZ, AT_RANDOM — not the user/group IDs. */
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

/* getisax — Solaris 9+ ISA extension query. Solaris 7 lacks it.
 * libsolcompat's src/hwcap.c provides a stub returning 0 (no
 * extensions), matching what microSPARC-II actually supports.
 * libgcc's libgcc/config/sparc/sol2-unwind.h calls this directly;
 * without the declaration the libgcc compile fails with implicit
 * declaration. Was previously handled by toolchain/patches.sh
 * Patch 7a — superseded by this declaration. */
#include <stdint.h>
extern int getisax(uint32_t *array, int n);

#ifdef __cplusplus
}
#endif

#endif /* _SOLCOMPAT_SYSROOT_SYS_AUXV_H */
