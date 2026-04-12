# libsolcompat — POSIX Compatibility Library for Solaris 7
#
# Builds with GCC (cross or native). No GNU make extensions required,
# but GNU make is recommended (gmake on Solaris).
#
# Cross-compile:  make CC=sparc-sun-solaris2.7-gcc AR=sparc-sun-solaris2.7-ar
# Native:         make
#
# Install targets:
#   make install                     — traditional install to PREFIX
#   make install-toolchain SYSROOT=  — scatter-install for transparent use
#
# The "scatter-install" approach puts objects into the right system libraries
# and override headers into a directory searched before /usr/include.
# After install-toolchain, './configure && make' just works — no special flags.

CC       ?= gcc
AR       ?= ar
RANLIB   ?= ranlib
NM       ?= nm
CFLAGS   ?= -O2 -Wall -Wextra -Wno-unused-parameter
CPPFLAGS ?= -D_REENTRANT -I$(CURDIR)/include
LDFLAGS  ?=
PICFLAGS  = -fPIC

PREFIX   ?= /usr/tgcware
LIBDIR    = $(PREFIX)/lib
INCDIR    = $(PREFIX)/include/solcompat
DOCDIR    = $(PREFIX)/share/doc/solcompat

SONAME    = libsolcompat.so.1
SOVERSION = 1.0.0

# Source files
SRCS = src/snprintf.c \
       src/string.c \
       src/stdio.c \
       src/memstream.c \
       src/stdlib.c \
       src/c99_types.c \
       src/network.c \
       src/clock.c \
       src/math.c \
       src/memory.c \
       src/filesystem.c \
       src/at_funcs.c \
       src/pty.c \
       src/process.c \
       src/poll.c \
       src/random.c \
       src/stubs.c \
       src/hwcap.c \
       src/xpg.c \
       src/getopt_long.c \
       src/ctype_compat.c \
       src/atomic_ops.c \
       src/fenv.c \
       src/cxa_atexit.c

OBJS     = $(SRCS:.c=.o)
PIC_OBJS = $(SRCS:.c=.lo)

# ====================================================================
# Object classification for scatter-install
# ====================================================================
# Objects merged into system libm.a (math functions)
LIBM_OBJS = src/math.o

# Objects merged into system libsocket.a (network functions)
LIBSOCKET_OBJS = src/network.o

# Objects merged into system libc.a (general POSIX/C99)
LIBC_OBJS = src/snprintf.o src/string.o src/stdio.o src/memstream.o \
            src/stdlib.o src/c99_types.o src/memory.o src/filesystem.o \
            src/at_funcs.o src/process.o src/pty.o src/poll.o src/random.o \
            src/clock.o src/stubs.o src/getopt_long.o src/ctype_compat.o \
            src/atomic_ops.o

# Residual libsolcompat.a (doesn't belong in any system library)
RESIDUAL_OBJS = src/xpg.o src/hwcap.o

# ====================================================================
# Build targets
# ====================================================================
all: libsolcompat.a

# Full monolithic library (used for development and traditional installs)
libsolcompat.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

# Static header/symbol consistency suite. Runs as part of the default
# build so CI picks it up automatically. Uses the same CC/CFLAGS/sysroot
# as the library build itself.
check: libsolcompat.a
	@CC="$(CC)" NM="$(NM)" \
	 CFLAGS="$(CFLAGS)" CPPFLAGS="$(CPPFLAGS)" \
	 LIB=libsolcompat.a \
	 sh tests/check_headers.sh

# Shared library — only built on request or when linking works.
#
# -nodefaultlibs: suppress GCC's default LIB_SPEC so we don't try to
# auto-link against -lsolcompat (which is the library we're currently
# building — chicken-and-egg). We explicitly list every library we
# actually need.
#
# -lgcc (static) only — NOT -lgcc_s. Some cross-toolchains are built
# with --disable-shared for libgcc (bootstrap.sh Phase 6a does this),
# which means libgcc_s.so.1 doesn't exist. libgcc.a has PIC objects
# (gcc builds libgcc PIC by default for use in shared library links
# even when --disable-shared is set), so we can link it statically
# into the shared solcompat library.
#
# -lc must come after -lgcc because libc references some libgcc
# runtime symbols indirectly.
$(SONAME): $(PIC_OBJS)
	$(CC) -shared -Wl,-h,$(SONAME) -Wl,-z,notext -nodefaultlibs \
	      -o $@ $(PIC_OBJS) $(LDFLAGS) \
	      -lgcc -lc -lgcc -lrt -lsocket -lnsl -lresolv -lm -ldl

# Compile rules
.SUFFIXES: .c .o .lo

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.c.lo:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PICFLAGS) -c -o $@ $<

# math.o needs -fno-builtin-fma* because GCC 15 treats the builtin
# declarations as implicit inline definitions, conflicting with our
# explicit implementations of fma/fmaf/fmal.
src/math.o: src/math.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -fno-builtin-fma -fno-builtin-fmaf -fno-builtin-fmal -c -o $@ $<
src/math.lo: src/math.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PICFLAGS) -fno-builtin-fma -fno-builtin-fmaf -fno-builtin-fmal -c -o $@ $<

# ====================================================================
# install — traditional PREFIX-based install (libsolcompat.a + headers)
# ====================================================================
install: all
	mkdir -p $(DESTDIR)$(LIBDIR)
	mkdir -p $(DESTDIR)$(INCDIR)
	mkdir -p $(DESTDIR)$(DOCDIR)
	cp libsolcompat.a $(DESTDIR)$(LIBDIR)/
	cp include/solcompat/*.h $(DESTDIR)$(INCDIR)/
	cp README.md $(DESTDIR)$(DOCDIR)/ 2>/dev/null || true
	cp LICENSE $(DESTDIR)$(DOCDIR)/ 2>/dev/null || true

# ====================================================================
# install-toolchain — Scatter-install for transparent ./configure use
# ====================================================================
#
# This target sets up a sysroot (or native system) so that standard
# build systems find everything automatically:
#
#   1. Override headers installed to SYSROOT/usr/local/include/
#      (GCC searches $sysroot/usr/local/include/ before $sysroot/usr/include/)
#   2. solcompat/ internal headers at SYSROOT/usr/local/include/solcompat/
#   3. Math objects merged into a COPY of libm.a in the override lib dir
#   4. Network objects merged into the override lib dir as libsocket additions
#   5. General POSIX objects merged into override libc additions
#   6. Residual libsolcompat.a for misc stubs (auto-linked via specs)
#   7. values-xpg6.o extracted for GCC specs
#   8. GCC specs file patched (if GCC_PREFIX given)
#
# After this, AC_CHECK_FUNC(fabsf) finds it in augmented libm.a,
# AC_CHECK_HEADER(stdint.h) finds it via the sysroot search order, and
# -lsolcompat is auto-linked into every binary.
#
# Usage:
#   make install-toolchain SYSROOT=/opt/sysroot-gcc11 [GCC_PREFIX=/opt/sst/gcc11]
#
# For cross-compilation (sysroot only, no specs patching):
#   make install-toolchain SYSROOT=/opt/sysroot-gcc11
#
# For native SPARC install (with specs patching):
#   make install-toolchain SYSROOT="" GCC_PREFIX=/opt/sst/gcc11

SYSROOT     ?= /opt/sysroot-gcc11
GCC_PREFIX  ?=
COMPAT_BASE  = $(SYSROOT)/opt/sst/lib/solcompat

install-toolchain: libsolcompat.a
	@echo ""
	@echo "=== libsolcompat scatter-install ==="
	@echo "  SYSROOT:     $(SYSROOT)"
	@echo "  COMPAT_BASE: $(COMPAT_BASE)"
	@echo ""
	@# --- Create directory structure ---
	mkdir -p $(SYSROOT)/usr/local/include/sys
	mkdir -p $(SYSROOT)/usr/local/include/arpa
	mkdir -p $(SYSROOT)/usr/local/include/net
	mkdir -p $(SYSROOT)/usr/local/include/netinet
	mkdir -p $(SYSROOT)/usr/local/include/solcompat
	mkdir -p $(COMPAT_BASE)/lib
	@# --- Install override wrapper headers ---
	@# GCC searches $sysroot/usr/local/include/ BEFORE $sysroot/usr/include/,
	@# so #include_next in overrides chains correctly to the real Solaris headers.
	@# No -isystem flags or specs patching needed.
	@echo "Installing override headers to sysroot /usr/local/include/..."
	cp include/override/*.h $(SYSROOT)/usr/local/include/
	@# Also copy extensionless C++ headers (cstdlib, etc.)
	@for f in include/override/*; do \
		case "$$f" in *.h) ;; */sys|*/arpa|*/net|*/netinet) ;; *) \
			[ -f "$$f" ] && cp "$$f" $(SYSROOT)/usr/local/include/; \
		esac; \
	done
	cp include/override/sys/*.h $(SYSROOT)/usr/local/include/sys/
	cp include/override/arpa/*.h $(SYSROOT)/usr/local/include/arpa/
	cp include/override/net/*.h $(SYSROOT)/usr/local/include/net/
	cp include/override/netinet/*.h $(SYSROOT)/usr/local/include/netinet/
	@echo "  override headers: math.h, stdint.h, netdb.h, sys/socket.h, etc."
	@# --- Install solcompat internal headers ---
	@echo "Installing solcompat headers..."
	cp include/solcompat/*.h $(SYSROOT)/usr/local/include/solcompat/
	@echo "  solcompat/*.h ($(words $(wildcard include/solcompat/*.h)) headers)"
	@# --- Merge math objects into augmented libm.a ---
	@echo "Augmenting libm.a with C99 math functions..."
	@if [ -f "$(SYSROOT)/usr/lib/libm.a" ]; then \
		cp "$(SYSROOT)/usr/lib/libm.a" $(COMPAT_BASE)/lib/libm.a; \
		$(AR) rs $(COMPAT_BASE)/lib/libm.a $(LIBM_OBJS); \
		echo "  Merged math.o into libm.a"; \
	else \
		echo "  No system libm.a found — creating standalone"; \
		$(AR) rcs $(COMPAT_BASE)/lib/libm.a $(LIBM_OBJS); \
	fi
	@# --- Merge network objects into augmented libsocket.a ---
	@echo "Augmenting libsocket.a with getaddrinfo/IPv6 stubs..."
	@if [ -f "$(SYSROOT)/usr/lib/libsocket.a" ]; then \
		cp "$(SYSROOT)/usr/lib/libsocket.a" $(COMPAT_BASE)/lib/libsocket.a; \
		$(AR) rs $(COMPAT_BASE)/lib/libsocket.a $(LIBSOCKET_OBJS); \
		echo "  Merged network.o into libsocket.a"; \
	else \
		echo "  No system libsocket.a found — creating standalone"; \
		$(AR) rcs $(COMPAT_BASE)/lib/libsocket.a $(LIBSOCKET_OBJS); \
	fi
	@# --- Create augmented libc supplement (can't easily merge into libc.a) ---
	@# Instead of merging into libc.a (which is huge and complex), we create
	@# a separate libsolcompat_c.a that specs will auto-link.
	@echo "Creating libc supplement (libsolcompat_c.a)..."
	$(AR) rcs $(COMPAT_BASE)/lib/libsolcompat_c.a $(LIBC_OBJS)
	@echo "  $(words $(LIBC_OBJS)) objects"
	@# --- Create residual libsolcompat.a ---
	@echo "Creating residual libsolcompat.a..."
	$(AR) rcs $(COMPAT_BASE)/lib/libsolcompat.a $(RESIDUAL_OBJS)
	@# --- Extract values-xpg6.o for GCC specs ---
	@echo "Creating values-xpg6.o..."
	cp src/xpg.o $(COMPAT_BASE)/lib/values-xpg6.o
	@# --- Install values-xpg6.o into sysroot /usr/lib too ---
	@if [ -d "$(SYSROOT)/usr/lib" ]; then \
		cp src/xpg.o "$(SYSROOT)/usr/lib/values-xpg6.o"; \
		echo "  values-xpg6.o installed to sysroot"; \
	fi
	@# --- Also install stdint.h directly into sysroot for GCC's fixincludes ---
	@if [ -d "$(SYSROOT)/usr/include" ] && [ ! -f "$(SYSROOT)/usr/include/stdint.h" ]; then \
		cp include/override/stdint.h "$(SYSROOT)/usr/include/stdint.h"; \
		echo "  stdint.h installed to $(SYSROOT)/usr/include/"; \
	fi
	@# --- Patch sysroot headers for fixincludes compatibility ---
	@# GCC's fixincludes rewrites sysroot headers into include-fixed/
	@# WITHOUT #include_next, completely shadowing our override directory.
	@# So we also append #include <solcompat/xxx.h> to sysroot headers
	@# and install solcompat headers into the sysroot's include path.
	@if [ -d "$(SYSROOT)/usr/include" ]; then \
		echo "Patching sysroot headers for fixincludes compatibility..."; \
		mkdir -p "$(SYSROOT)/usr/include/solcompat"; \
		cp include/solcompat/*.h "$(SYSROOT)/usr/include/solcompat/"; \
		for pair in \
			"math.h:solcompat/math_ext.h" \
			"stdlib.h:solcompat/stdlib_ext.h" \
			"string.h:solcompat/string_ext.h" \
			"stdio.h:solcompat/stdio_ext.h" \
			"inttypes.h:solcompat/c99_types.h" \
		; do \
			hdr="$${pair%%:*}"; ext="$${pair#*:}"; \
			target="$(SYSROOT)/usr/include/$${hdr}"; \
			if [ -f "$${target}" ] && ! grep -q "$${ext}" "$${target}"; then \
				echo "" >> "$${target}"; \
				echo "/* libsolcompat: extensions for Solaris 7 */" >> "$${target}"; \
				echo "#include <$${ext}>" >> "$${target}"; \
				echo "  Patched $${hdr} → $${ext}"; \
			fi; \
		done; \
		echo "  Sysroot headers patched"; \
	fi
	@# --- Patch GCC specs if GCC_PREFIX is provided ---
	@# NOTE: Specs patching for -isystem is no longer needed since override
	@# headers are installed to $sysroot/usr/local/include/ which GCC
	@# searches automatically.  The specs patch is only for -lsolcompat
	@# auto-linking (LIB_SPEC in sol2.h handles this for the native compiler).
	@if [ -n "$(GCC_PREFIX)" ]; then \
		echo "Patching GCC specs (library paths only)..."; \
		sh scripts/patch-specs.sh "$(GCC_PREFIX)" \
			"$(SYSROOT)/usr/local/include" \
			"$(COMPAT_BASE)/lib"; \
	else \
		echo ""; \
		echo "GCC_PREFIX not set — skipping specs patching."; \
		echo "Override headers are at $(SYSROOT)/usr/local/include/ (found automatically)."; \
		echo "For library linking: -L$(COMPAT_BASE)/lib -lsolcompat -lsolcompat_c"; \
	fi
	@# --- Summary ---
	@echo ""
	@echo "=== Scatter-install complete ==="
	@echo "  Override headers: $(SYSROOT)/usr/local/include/"
	@echo "  Solcompat headers: $(SYSROOT)/usr/local/include/solcompat/"
	@echo "  Augmented libm:  $(COMPAT_BASE)/lib/libm.a"
	@echo "  Augmented libsocket: $(COMPAT_BASE)/lib/libsocket.a"
	@echo "  libc supplement: $(COMPAT_BASE)/lib/libsolcompat_c.a"
	@echo "  Residual compat: $(COMPAT_BASE)/lib/libsolcompat.a"
	@echo "  values-xpg6.o:  $(COMPAT_BASE)/lib/values-xpg6.o"
	@echo ""

# ====================================================================
# install-headers — compiler-free subset of install-toolchain
# ====================================================================
# Installs override headers, solcompat internal headers, and the
# sysroot-overlay (stdint.h) WITHOUT requiring any of the .o files
# built by `all`. Safe to run before a cross compiler exists,
# e.g. during toolchain bootstrap Phase 4 (writable sysroot setup)
# where there is nothing yet that can compile for the target.
#
# Usage:
#   make install-headers SYSROOT=/path/to/sysroot
#
# After the cross compiler is available, run `make install-toolchain`
# for the full install (augmented libm/libsocket/libc + specs patches).
install-headers:
	@echo ""
	@echo "=== libsolcompat install-headers ==="
	@echo "  SYSROOT:     $(SYSROOT)"
	@echo ""
	@# --- Create directory structure ---
	mkdir -p $(SYSROOT)/usr/local/include/sys
	mkdir -p $(SYSROOT)/usr/local/include/arpa
	mkdir -p $(SYSROOT)/usr/local/include/net
	mkdir -p $(SYSROOT)/usr/local/include/netinet
	mkdir -p $(SYSROOT)/usr/local/include/solcompat
	@# --- Install override wrapper headers ---
	@# GCC searches $sysroot/usr/local/include/ BEFORE $sysroot/usr/include/,
	@# so #include_next chains correctly to the real Solaris headers.
	@echo "Installing override headers to sysroot /usr/local/include/..."
	cp include/override/*.h $(SYSROOT)/usr/local/include/
	@# Also copy extensionless C++ headers (cstdlib, etc.)
	@for f in include/override/*; do \
		case "$$f" in *.h) ;; */sys|*/arpa|*/net|*/netinet) ;; *) \
			[ -f "$$f" ] && cp "$$f" $(SYSROOT)/usr/local/include/; \
		esac; \
	done
	cp include/override/sys/*.h $(SYSROOT)/usr/local/include/sys/
	cp include/override/arpa/*.h $(SYSROOT)/usr/local/include/arpa/
	cp include/override/net/*.h $(SYSROOT)/usr/local/include/net/
	cp include/override/netinet/*.h $(SYSROOT)/usr/local/include/netinet/
	@# --- Install solcompat internal headers ---
	@echo "Installing solcompat headers..."
	cp include/solcompat/*.h $(SYSROOT)/usr/local/include/solcompat/
	@# --- Install stdint.h directly into sysroot for GCC's fixincludes ---
	@if [ -d "$(SYSROOT)/usr/include" ] && [ ! -f "$(SYSROOT)/usr/include/stdint.h" ]; then \
		cp include/override/stdint.h "$(SYSROOT)/usr/include/stdint.h"; \
		echo "  stdint.h installed to $(SYSROOT)/usr/include/"; \
	fi
	@# --- Install sysroot-overlay (anything else that ships pre-built) ---
	@if [ -d sysroot-overlay ]; then \
		cp -r sysroot-overlay/* "$(SYSROOT)/"; \
		echo "  sysroot-overlay installed"; \
	fi
	@# --- Mirror solcompat headers into sysroot /usr/include/solcompat ---
	@# Needed because fixincludes-patched copies in include-fixed/ use
	@# #include <solcompat/xxx.h> which resolves via /usr/include/.
	@if [ -d "$(SYSROOT)/usr/include" ]; then \
		mkdir -p "$(SYSROOT)/usr/include/solcompat"; \
		cp include/solcompat/*.h "$(SYSROOT)/usr/include/solcompat/"; \
		echo "  solcompat/*.h mirrored into sysroot /usr/include"; \
	fi
	@echo ""
	@echo "=== install-headers complete ==="
	@echo "  Override headers: $(SYSROOT)/usr/local/include/"
	@echo "  No -isystem flags needed — GCC finds them via the sysroot automatically."
	@echo "  (Run install-toolchain after cross compiler is built for the full install)"
	@echo ""

# ====================================================================
# install-sysroot — Legacy: simple sysroot overlay install
# ====================================================================
install-sysroot: all
	@echo "Installing sysroot overlay headers to $(SYSROOT)..."
	@if [ -d sysroot-overlay ]; then \
		cp -r sysroot-overlay/* $(SYSROOT)/; \
		echo "  Sysroot overlay installed"; \
	fi
	@echo "Installing libsolcompat into sysroot..."
	mkdir -p $(SYSROOT)/usr/lib $(SYSROOT)/usr/include/solcompat
	cp libsolcompat.a $(SYSROOT)/usr/lib/
	cp include/solcompat/*.h $(SYSROOT)/usr/include/solcompat/
	@echo "  libsolcompat installed to $(SYSROOT)/usr/lib/"

# ====================================================================
# Tests (run on Solaris target or under QEMU)
# ====================================================================
test: all
	cd tests && $(CC) $(CPPFLAGS) $(CFLAGS) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 \
		-o test_all test_all.c \
		-L$(CURDIR) -lsolcompat -lrt -lsocket -lnsl -lm -ldl && ./test_all

# ====================================================================
# Clean
# ====================================================================
clean:
	rm -f $(OBJS) $(PIC_OBJS) libsolcompat.a $(SONAME)
	rm -f tests/test_all
	rm -rf tests/gen

.PHONY: all check install install-toolchain install-headers install-sysroot test clean
