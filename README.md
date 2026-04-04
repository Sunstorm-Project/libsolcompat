# libsolcompat

POSIX.1-2024 compatibility library for Solaris 7 SPARC.

libsolcompat bridges the gap between Solaris 7's SUSv2/POSIX.1-1997 APIs and the POSIX.1-2024 (Issue 8) standard. It provides 300+ functions across 23 source files that are missing from Solaris 7 but required by current versions of GCC, GNU coreutils, Python, OpenSSL, git, SDL2, ScummVM, and other tools.

Part of the [Sunstorm Project](https://github.com/Sunstorm-Project). Installed as Wave 0 in the SST build pipeline — every subsequent package links against it.

## What It Provides

| Module | Functions |
|--------|-----------|
| **snprintf** | C99-conformant `snprintf`/`vsnprintf` (Solaris 7 returns -1 on truncation) |
| **string** | `strndup`, `strnlen`, `strlcpy`, `strlcat`, `strcasestr`, `memmem`, `strsep`, `stpcpy`, `stpncpy`, `strchrnul`, `memrchr`, `strsignal`, `strerror_r` (GNU-compatible) |
| **stdio** | `getline`, `getdelim`, `vasprintf`, `asprintf`, `dprintf`, `fmemopen`, `open_memstream`, `preadv`, `pwritev` |
| **stdlib** | `setenv`, `unsetenv`, `mkdtemp`, `_Exit`, `strtoimax`, `strtoumax`, `strtof`, `strtold`, `atoll`, `qsort_r`, `imaxabs`, `imaxdiv` |
| **c99_types** | `SCN*` scan format macros, `INTMAX_C`/`UINTMAX_C` |
| **network** | `getaddrinfo`/`freeaddrinfo`/`gai_strerror`/`getnameinfo`, `inet_ntop`/`inet_pton`, full IPv6 types (`struct sockaddr_in6`, `struct sockaddr_storage`, `IN6_IS_ADDR_*`), `getifaddrs`/`freeifaddrs`, `if_nametoindex`/`if_indextoname`/`if_nameindex`, `accept4`, `sockatmark` |
| **clock** | `CLOCK_MONOTONIC` (via `gethrtime()`), `CLOCK_PROCESS_CPUTIME_ID`, `CLOCK_THREAD_CPUTIME_ID`, `clock_nanosleep`, `timegm` |
| **math** | 100+ C99 functions: all float (`fabsf`, `sinf`, ...) and long double (`fabsl`, `sinl`, ...) variants, `round`, `trunc`, `log2`, `exp2`, `fdim`, `fmin`, `fmax`, `tgamma`, `nearbyint`, `lrint`/`llrint`, `lround`/`llround`, `nan`/`nanf`/`nanl`, `fpclassify`, `NAN`, `INFINITY`, `cexp` |
| **fenv** | Full C99 `<fenv.h>`: `fegetround`/`fesetround`, `feclearexcept`/`feraiseexcept`/`fetestexcept`, `fegetenv`/`fesetenv`, `feholdexcept`/`feupdateenv` — real SPARC FSR implementation |
| **memory** | `posix_memalign`, `aligned_alloc`, `reallocarray`, `posix_madvise`, `MAP_ANONYMOUS` wrapper (redirects through `/dev/zero`) |
| **filesystem** | `utimes`, `futimens`, `utimensat`, `flock`, `scandir`/`alphasort`, `fdopendir`, `dirfd`, `posix_fadvise`, `posix_fallocate` |
| **at_funcs** | `openat`, `mkdirat`, `renameat`, `unlinkat`, `fstatat`, `fchownat`, `fchmodat`, `readlinkat`, `symlinkat`, `linkat`, `faccessat` (mutex-protected `fchdir` emulation) |
| **pty** | `posix_openpt`, `openpty`, `forkpty`, `login_tty`, `cfmakeraw` |
| **process** | `daemon`, `err`/`warn`/`errx`/`warnx`, `posix_spawn`/`posix_spawnp` (full file_actions + spawnattr), `pipe2`, `dup3`, `mkostemp`, `execvpe`, `getgrouplist` |
| **poll** | `ppoll`, `pselect` |
| **random** | `explicit_bzero`, `getentropy`, `arc4random`/`arc4random_buf`/`arc4random_uniform` |
| **ctype** | `isblank`, locale-aware `_l` variants: `isalnum_l`, `isalpha_l`, `isblank_l`, `iscntrl_l`, `isdigit_l`, `isgraph_l`, `islower_l`, `isprint_l`, `ispunct_l`, `isspace_l`, `isupper_l`, `isxdigit_l`, `tolower_l`, `toupper_l` |
| **locale** | `newlocale`/`uselocale`/`freelocale`/`duplocale`, `locale_t`, `LC_*_MASK`, `LC_GLOBAL_LOCALE`, `nl_langinfo_l` |
| **threading** | `pthread_setname_np`, `sem_timedwait`, `pthread_condattr_getclock`/`setclock`, `pthread_attr_getstack`/`setstack` |
| **mqueue** | `mq_timedreceive`, `mq_timedsend` |
| **getopt** | `getopt_long`, `getopt_long_only` |
| **atomic** | GCC `__atomic_*` builtins (load/store/exchange/CAS/fetch_add/fetch_or/...) for 1/4/8-byte types, plus Solaris-style `atomic_add_int`/`atomic_cas_uint`/`atomic_swap_ptr` |
| **hwcap** | `getisax()` stub (Solaris 9+ feature) |
| **xpg** | `__xpg6` symbol, `KSTAT_DATA_STRING` constants |
| **override headers** | 26 wrapper headers shadowing Solaris 7 system headers: `stdint.h`, `inttypes.h`, `stdio.h`, `stdlib.h`, `string.h`, `math.h`, `fenv.h`, `time.h`, `ctype.h`, `fcntl.h`, `poll.h`, `spawn.h`, `dirent.h`, `locale.h`, `endian.h`, `atomic.h`, `sys/mman.h`, `sys/socket.h`, `net/if.h`, `netinet/in.h`, `netinet/in6.h`, `arpa/inet.h` |

## Building

### Cross-compile (from x86_64 Linux host using the toolchain container)

```sh
make \
  CC=sparc-sun-solaris2.7-gcc \
  AR=sparc-sun-solaris2.7-ar \
  RANLIB=sparc-sun-solaris2.7-ranlib \
  CFLAGS="-O2 --sysroot=/opt/sysroot-build -mcpu=v7 -D__EXTENSIONS__"
```

Cross-compiler lives at `/opt/cross/bin/sparc-sun-solaris2.7-*` inside the toolchain container. Sysroot is at `/opt/sysroot-build`.

### Native (on Solaris 7 SPARC)

```sh
gmake
```

### Install

```sh
gmake install DESTDIR=/path/to/staging PREFIX=/opt/sst
```

This installs:
- `lib/libsolcompat.a` — static library
- `lib/libsolcompat.so.1` — shared library
- `include/solcompat/*.h` — 21 implementation headers
- `include/override/*.h` — 26 override headers (shadow system headers)

## Usage

### Automatic (recommended)

```sh
CPPFLAGS="-I/opt/sst/include -include solcompat/solcompat.h"
LDFLAGS="-L/opt/sst/lib -lsolcompat"
```

The `-include` flag automatically overrides broken libc functions (such as `snprintf`) and provides all missing prototypes.

### Selective

```c
#include <solcompat/snprintf.h>
#include <solcompat/string_ext.h>
```

## CI

`.github/workflows/ci.yml` runs on push to `master` and on tag push (`v*`).

Three jobs:

1. **cross-compile** — Pulls toolchain image, builds `libsolcompat.a` and `libsolcompat.so.1`, verifies symbol table.

2. **qemu-test** — Cross-compiles test binary, boots Solaris 7 VM via QEMU, transfers and runs `test_all` natively.

3. **release** — Creates GitHub Release on `v*` tag push. Packages library + headers into `libsolcompat-<version>-sparc-solaris7.tar.gz`.

## POSIX Coverage Auditing

The SST build pipeline includes `scripts/audit-posix-coverage.sh` which compares Solaris 7 system libraries + libsolcompat against the POSIX.1-2024 (Issue 8) mandatory function list to identify remaining gaps. Run it with the sysroot for accurate results:

```sh
./scripts/audit-posix-coverage.sh \
  --sysroot /opt/sysroot \
  --libsolcompat /path/to/libsolcompat \
  --nm /opt/cross/bin/sparc-sun-solaris2.7-nm
```

## Testing

```sh
gmake test
```

Runs `tests/test_all.c` covering all subsystems. Requires Solaris 7 SPARC (or QEMU). In CI this is handled by the `qemu-test` job.

## Design Notes

- **Thread safety**: The `*at()` functions use a global mutex + `fchdir()` dance. Single-threaded code is unaffected; multi-threaded code gets serialized through these specific calls.
- **IPv6**: Emulated over IPv4 via `gethostbyname`. Solaris 7 has experimental IPv6 kernel support but we don't rely on it. Full `struct sockaddr_in6` and `IN6_IS_ADDR_*` macros are provided for compilation compatibility.
- **No ELF TLS**: Solaris 7's linker doesn't support `__thread`. Use `--disable-tls` when building GCC (which activates emutls in libgcc).
- **Random quality**: If SUNWski is installed, `/dev/random` provides real entropy. Otherwise falls back to a PRNG seeded from `gethrtime()`.
- **fenv.h**: Real SPARC FSR (floating-point status register) implementation via inline assembly, not stubs.
- **Locale**: Per-thread locale (`newlocale`/`uselocale`) delegates to global `setlocale()`. The `_l` ctype variants ignore the locale argument. This is sufficient for the "temporarily switch to C locale" pattern used by most software.
- **MAP_ANONYMOUS**: Solaris 7 kernel has no anonymous mmap. The wrapper opens `/dev/zero` and uses it as the backing fd — transparent to callers.

## Target Environment

- Solaris 7 (SunOS 5.7) SPARC
- GCC cross-compiler (`sparc-sun-solaris2.7-gcc`) or native GCC
- Install prefix: `/opt/sst`

## License

MIT — see [LICENSE](LICENSE)
