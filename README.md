# libsolcompat

POSIX/C99/C11 compatibility library for Solaris 7 SPARC.

libsolcompat bridges the gap between Solaris 7's SUSv2/POSIX.1-1997 APIs and what modern (2026-era) open source software expects. It provides 250+ functions across 21 source files that are missing from Solaris 7 but required by current versions of GCC, GNU coreutils, Python, OpenSSL, git, and other foundational tools.

Public repo: github.com/firefly128/libsolcompat — v1.0.0 released.

Used by the Sunstorm distribution as Wave 0: installed first, and every subsequent package links against it.

## What It Provides

| Module | Functions |
|--------|-----------|
| **snprintf** | C99-conformant `snprintf`/`vsnprintf` (Solaris 7 returns -1 on truncation) |
| **string** | `strndup`, `strnlen`, `strlcpy`, `strlcat`, `strcasestr`, `memmem`, `strsep`, `stpcpy`, `stpncpy`, `strchrnul`, `memrchr`, `strtoimax`, `strtoumax` |
| **stdio** | `getline`, `getdelim`, `vasprintf`, `asprintf`, `dprintf` |
| **stdlib** | `setenv`, `unsetenv`, `mkdtemp`, `qsort_r` |
| **c99_types** | `SCN*` scan format macros, `INTMAX_C`/`UINTMAX_C` |
| **network** | `getaddrinfo`/`freeaddrinfo`/`gai_strerror`/`getnameinfo`, `inet_ntop`/`inet_pton`, `struct sockaddr_storage`, `getifaddrs`/`freeifaddrs` |
| **clock** | `CLOCK_MONOTONIC` (via `gethrtime()`), `CLOCK_PROCESS_CPUTIME_ID`, `clock_nanosleep` |
| **math** | `round`, `trunc`, `log2`, `exp2`, `fdim`, `fmin`, `fmax`, `fpclassify`, `NAN`, `INFINITY` |
| **memory** | `posix_memalign`, `aligned_alloc`, `reallocarray`, `MAP_ANONYMOUS` wrapper |
| **filesystem** | `utimes`, `futimens`, `utimensat`, `flock`, `scandir`/`alphasort`, `fdopendir`, `posix_fadvise` |
| **at_funcs** | `openat`, `mkdirat`, `renameat`, `unlinkat`, `fstatat`, `fchownat`, `fchmodat`, `readlinkat`, `symlinkat`, `linkat`, `faccessat` (mutex-protected fchdir emulation) |
| **pty** | `posix_openpt`, `openpty`, `forkpty`, `login_tty`, `cfmakeraw` |
| **process** | `daemon`, `err`/`warn`/`errx`/`warnx`, `posix_spawn`/`posix_spawnp`, `pipe2`, `dup3`, `mkostemp`, `execvpe` |
| **poll** | `ppoll`, `pselect` |
| **random** | `explicit_bzero`, `getentropy`, `arc4random`/`arc4random_buf`/`arc4random_uniform` |
| **stubs** | `pthread_setname_np`, `newlocale`/`uselocale`/`freelocale`/`duplocale` |
| **dirent** | Override header providing `dirfd` and `DIR.dd_fd` access |
| **unistd** | Override header providing missing `_POSIX_*` constants and prototypes |

## Building

### Cross-compile (from x86_64 Linux host using the toolchain container)

```sh
make \
  CC=sparc-sun-solaris2.7-gcc \
  AR=sparc-sun-solaris2.7-ar \
  RANLIB=sparc-sun-solaris2.7-ranlib \
  CFLAGS="-O2 --sysroot=/opt/sysroot-build -mcpu=v7 -D__EXTENSIONS__"
```

Cross-compiler lives at `/opt/cross/bin/sparc-sun-solaris2.7-*` inside the `ghcr.io/firefly128/sparc-toolchain:latest` container. Sysroot is at `/opt/sysroot-build`.

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
- `include/solcompat/*.h` — headers
- `include/override/*.h` — override headers for dirent.h and unistd.h

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

1. **cross-compile** — Runs on `sparc-qemu` self-hosted runner. Pulls `ghcr.io/firefly128/sparc-toolchain:latest`, builds `libsolcompat.a`, and verifies the symbol table. Uses the `sparc-build-host_cross-build-cross` Docker volume for the cross-compiler and the sysroot from `/opt/sparc/sparc-build-host/cross-build/sysroot/`.

2. **qemu-test** — Runs on `sparc-qemu` runner (.244 only — the machine with Solaris disk images). Cross-compiles the test binary, boots the Solaris VM via `docker compose --profile runtime up -d`, transfers the binary via uuencode over the QEMU serial console, runs `test_all` natively on Solaris 7, and shuts QEMU down.

3. **release** — Creates a GitHub Release on `v*` tag push. Packages the library and headers into a tarball named `libsolcompat-<version>-sparc-solaris7.tar.gz`.

## Testing

```sh
gmake test
```

Runs the test suite covering all subsystems. Requires running on Solaris 7 SPARC (or QEMU emulating one). In CI this is handled by the `qemu-test` job.

## Design Notes

- **Thread safety**: The `*at()` functions use a global mutex + `fchdir()` dance. Single-threaded code is unaffected; multi-threaded code gets serialized through these specific calls.
- **IPv4 only**: The networking module uses `gethostbyname` under the hood. Solaris 7 has experimental IPv6 but we don't rely on it.
- **No ELF TLS**: Solaris 7's linker doesn't support `__thread`. Use `--disable-tls` when building GCC (which activates emutls in libgcc).
- **Random quality**: If SUNWski is installed, `/dev/random` provides real entropy. Otherwise falls back to a PRNG seeded from `gethrtime()`.

## Target Environment

- Solaris 7 (SunOS 5.7) SPARC
- GCC 11.4.0 cross-compiler (`sparc-sun-solaris2.7-gcc`) or native GCC
- Install prefix: `/opt/sst`

## License

MIT — see [LICENSE](LICENSE)
