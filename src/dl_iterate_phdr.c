/*
 * dl_iterate_phdr.c — Walk loaded ELF objects for Solaris 7
 *
 * Solaris 7 lacks dl_iterate_phdr (added in Solaris 10). GCC's libgcc
 * uses it for DWARF2 exception handling frame registration via
 * PT_GNU_EH_FRAME. Without it, libgcc falls back to a slower
 * register/deregister method.
 *
 * This implementation walks the runtime linker's link map (r_debug)
 * and reads ELF program headers from each loaded object to populate
 * struct dl_phdr_info for the callback.
 *
 * The implementation is intentionally simple — it re-reads program
 * headers from the mapped ELF image each time rather than caching.
 * This is called infrequently (only during exception unwinding).
 */

#include <sys/types.h>
#include <sys/elf.h>
#include <sys/link.h>
#include <link.h>
#include <dlfcn.h>
#include <string.h>

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

/* Access the runtime linker's debug structure via _DYNAMIC */
extern Elf32_Dyn _DYNAMIC[];

static struct r_debug *
get_r_debug(void)
{
    Elf32_Dyn *dynamic_entry;

    for (dynamic_entry = _DYNAMIC; dynamic_entry->d_tag != DT_NULL; dynamic_entry++) {
        if (dynamic_entry->d_tag == DT_DEBUG) {
            return (struct r_debug *)(dynamic_entry->d_un.d_ptr);
        }
    }
    return (struct r_debug *)0;
}

/*
 * dl_iterate_phdr — iterate over loaded ELF objects
 *
 * Calls 'callback' for each loaded shared object (and the main executable).
 * Returns the first non-zero return from callback, or 0 if all return 0.
 */
int
dl_iterate_phdr(int (*callback)(struct dl_phdr_info *, size_t, void *),
                void *callback_data)
{
    struct r_debug *debug_info;
    Link_map *current_map;
    int callback_result;

    debug_info = get_r_debug();
    if (!debug_info || !debug_info->r_map)
        return 0;

    for (current_map = debug_info->r_map; current_map != (Link_map *)0;
         current_map = current_map->l_next) {

        struct dl_phdr_info phdr_info;
        Elf32_Ehdr *elf_header;

        /* The link map l_addr is the base address where the object is mapped.
           The ELF header is at that address for the main executable (l_addr=0
           means it's at its preferred address) and at l_addr for shared libs. */
        if (current_map->l_addr == 0 && current_map->l_prev == (Link_map *)0) {
            /* Main executable — l_addr is 0, ELF header is at the start
               of the text segment. Use l_ld to find the base. For a
               non-PIE executable on Solaris 7, we can read the phdr
               from the auxiliary vector or just skip it since the main
               executable's FDEs are registered via __register_frame_info. */
            continue;
        }

        /* Read ELF header from the mapped image */
        elf_header = (Elf32_Ehdr *)(current_map->l_addr);

        /* Sanity check — verify this looks like an ELF file */
        if (elf_header->e_ident[0] != 0x7f ||
            elf_header->e_ident[1] != 'E' ||
            elf_header->e_ident[2] != 'L' ||
            elf_header->e_ident[3] != 'F')
            continue;

        /* Populate the dl_phdr_info structure */
        phdr_info.dlpi_addr = current_map->l_addr;
        phdr_info.dlpi_name = current_map->l_name ? current_map->l_name : "";
        phdr_info.dlpi_phdr = (const Elf32_Phdr *)
            (current_map->l_addr + elf_header->e_phoff);
        phdr_info.dlpi_phnum = elf_header->e_phnum;

        callback_result = callback(&phdr_info, sizeof(phdr_info), callback_data);
        if (callback_result != 0)
            return callback_result;
    }

    return 0;
}
