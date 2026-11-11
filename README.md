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
| **sysroot headers** | Installed via `sysroot-overlay/` (wholly new headers Solaris 7 lacks: `stdint.h`, `endian.h`, `fenv.h`, `getopt.h`, `atomic.h`, `ifaddrs.h`, `error.h`, `complex.h`, `mntent.h`, `pty.h`, `spawn.h`, `stdatomic.h`, `stdbool.h`, `threads.h`, `uchar.h`, `ucred.h`, `xlocale.h`, `netinet/in6.h`, `sys/auxv.h`, `sys/random.h`, `sys/soundcard.h`, `sys/xattr.h`) and `sysroot-prep/*.append`/`*.prepend` (POSIX/C99 additions appended to pristine Solaris 7 headers: `stdio.h`, `stdlib.h`, `string.h`, `math.h`, `time.h`, `locale.h`, `sys/mman.h`, `sys/socket.h`, `netdb.h`, …) |

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
- `include/solcompat/*.h` — implementation headers for the solcompat-extension API

Separately, `make install-headers` applies the `sysroot-overlay/` and `sysroot-prep/` trees to the cross-sysroot so pristine Solaris 7 headers gain POSIX/C99 declarations that the libsolcompat `.a`/`.so.1` back at link time. This is driven by `toolchain/bootstrap.sh` in the sst-build-pipeline and is not normally invoked standalone.

## Usage

### Cross-compile (sst-build-pipeline)

The pipeline's `toolchain/bootstrap.sh` runs `make install-headers` into the writable sysroot and links `libsolcompat.so.1` into `cc1`'s `DT_NEEDED` via `patches.sh` Patch 7 (`LIB_SPEC`). No per-package configuration is needed; every target binary picks up libsolcompat automatically.

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


### Formatting and mount ABI regression checks

The C99 output family uses a bounded output sink and an MIT-licensed formatting
core derived from musl v1.2.5. The copyright and upstream source URL are retained
in `src/snprintf.c`. The same core implements stream and string entry points,
including positional arguments and C99 integer lengths. `frexpl` preserves the
actual SPARC long-double precision and exponent range instead of narrowing it.

GNU `<mntent.h>` callers use prefixed compatibility exports. Native Solaris
`<sys/mnttab.h>` callers retain their two-argument `getmntent` ABI; that symbol
is never replaced with an incompatible one-argument function.

`make compat-tests` builds focused formatting, bounds, long-double and both
mount-ABI regression binaries. `make test-compat` runs them on the target using
the just-built DSO. For cross builds, transfer those binaries and the DSO to an
isolated Solaris system and set a command-local library search path to that
private directory. Do not execute SPARC test binaries on the build host.

These tests cover byte bounds, long formats/output, C99 j/z/t/hh conversions,
positional width/precision, stream output, binary128 large/small values and GNU
and native mount enumeration. Toolchain constructor/destructor/exception
qualification is separate; passing the C API tests does not qualify C++ startup
or unwinding. Diagnostic sources under `tests/probes` help isolate those stages.

### Native filesystem and loader contracts

`getcwd(NULL, 0)` allocates and grows a buffer for GNU callers. Positive-size
allocation and caller-supplied buffers retain native Solaris behavior through
`_getcwd`. Mount enumeration decodes escaped whitespace and backslashes once.
For non-PIE executables, `dl_iterate_phdr` reports zero load bias so consumers
interpret program-header virtual addresses correctly.

Timestamp wrappers validate arguments and preserve files on unsupported calls.
Descriptor operations unavailable through Solaris return `ENOSYS` so callers can
use their pathname fallback. Unsupported no-follow timestamp updates also fail;
they never silently update a symlink's target. These wrappers do not provide
complete POSIX timestamp support.

Focused target tests include loader address calculations, native/GNU getcwd
allocation and growth, and timestamp success/failure behavior. Host checks
`python3 tests/test_format_attributes.py` and `python3 tests/test_header_prepend.py`
verify compiler format diagnostics and safe insertion into native header guards.

### Native compiler header coverage

The cross sysroot already contains the compatibility declarations. A native
compiler also needs them after installation, even for headers that GCC's
fixincludes did not copy. During package staging, run:

```sh
make install-native-headers NATIVE_INCLUDE_DIR=/staging/path/to/gcc/include-fixed
```

This host build step requires Python3. It creates authored `include_next`
forwarders for missing C headers and installs complete authored C overlays.
Existing vendor fixed headers remain for the pipeline's guarded-block refresher;
no vendor header is copied by this generator. C++ overlays use the separate
`install-cxx-overlay` target. The resulting compiler package needs no Python
for header lookup on Solaris.

The socket header does not force `_XPG4_2`: Solaris7 still uses its legacy
message-header layout, and forcing that feature macro changes unrelated vendor
declarations. The authoritative socket append supplies the supported API.

C++ compatibility wrappers retain the original upstream `cmath` and `cstdlib`
under `.solcompat-upstream` in the compiler header tree, then include those
originals explicitly. The retained originals are part of the packaged header
payload. A previously overwritten header with no recoverable original is an
error requiring a fresh compiler-header installation; the installer never
silently substitutes a wrapper for the upstream implementation.

For libstdc++'s generated build headers, run `install-cxx-overlay` after its
configure and include-header generation, before compiling runtime objects. Set
`CXX_INCLUDE_DIR` to the generated include directory and `CXX_SOURCE_ROOT` to
the GCC source tree. Generated links are accepted only beneath that explicit
source root, copied into retained originals, and replaced atomically without
writing through to GCC sources. Install rules must preserve raw upstream header
bytes before the installed-prefix overlay is applied. The C++ math facade clears
false native C++11 classifier-prototype claims after loading `c++config.h`; the
Solaris C header still lacks those overloads. This is a C++ header correction,
not a change to the C runtime or Perl's captured C configuration.
