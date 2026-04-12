/*
 * solcompat/dl_phdr_info.h — struct dl_phdr_info for Solaris 7
 *
 * Solaris 7 lacks struct dl_phdr_info and dl_iterate_phdr() (added in
 * Solaris 10).  This header provides the struct definition and function
 * declaration for both internal libsolcompat use and external consumers
 * (via sysroot patching of sys/link.h).
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */
#ifndef SOLCOMPAT_DL_PHDR_INFO_H
#define SOLCOMPAT_DL_PHDR_INFO_H

#ifdef __sun

#include <sys/types.h>
#include <sys/elf.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * struct dl_phdr_info — matches the Linux/Solaris 10+ ABI.
 * GCC's unwind code checks the size parameter to determine which
 * fields are available.
 */
struct dl_phdr_info {
    Elf32_Addr        dlpi_addr;    /* Base address of object */
    const char       *dlpi_name;    /* Name of object */
    const Elf32_Phdr *dlpi_phdr;    /* Pointer to array of program headers */
    Elf32_Half        dlpi_phnum;   /* Number of program headers */
};

/*
 * dl_iterate_phdr — iterate over loaded ELF objects.
 * Calls 'callback' for each loaded shared object (and the main executable).
 * Returns the first non-zero return from callback, or 0 if all return 0.
 */
int dl_iterate_phdr(int (*callback)(struct dl_phdr_info *, size_t, void *),
                    void *data);

#ifdef __cplusplus
}
#endif

#endif /* __sun */

#endif /* SOLCOMPAT_DL_PHDR_INFO_H */
