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
       src/wchar.c \
       src/stubs.c \
       src/hwcap.c \
       src/xpg.c \
       src/getopt_long.c \
       src/ctype_compat.c \
       src/atomic_ops.c \
       src/fenv.c \
       src/cxa_atexit.c \
       src/dl_iterate_phdr.c \
       src/complex_math.c \
       src/ssp.c

OBJS     = $(SRCS:.c=.o)
PIC_OBJS = $(SRCS:.c=.lo)

# ====================================================================
# Object classification for scatter-install
# ====================================================================
# Objects merged into system libm.a (math functions)
LIBM_OBJS = src/math.o src/complex_math.o src/fenv.o

# Objects merged into system libsocket.a (network functions)
LIBSOCKET_OBJS = src/network.o

# Objects merged into system libc.a (general POSIX/C99)
LIBC_OBJS = src/snprintf.o src/string.o src/stdio.o src/memstream.o \
            src/stdlib.o src/c99_types.o src/memory.o src/filesystem.o \
            src/at_funcs.o src/process.o src/pty.o src/poll.o src/random.o \
            src/clock.o src/stubs.o src/getopt_long.o src/ctype_compat.o \
            src/atomic_ops.o src/wchar.o src/cxa_atexit.o \
            src/dl_iterate_phdr.o src/ssp.o

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
# install-toolchain — GNU-standard sysroot setup for transparent builds
# ====================================================================
#
# Makes the sysroot look like a standard GNU system. After this,
# ./configure && make just works — no special flags or paths needed.
#
#   1. sysroot-overlay: complete headers for APIs Solaris 7 lacks
#      (stdint.h, fenv.h, spawn.h, etc.) → $SYSROOT/usr/include/
#   2. sysroot-prep: declarations appended to existing system headers
#      (C99 math, POSIX string, IPv6, etc.) → $SYSROOT/usr/include/*.h
#   3. sys/int_types.h replaced with stdint.h redirect
#   4. C++ headers (cmath, cstdlib) installed to sysroot
#   5. Augmented system libraries (libm.a, libsocket.a, libc supplement)
#   6. values-xpg6.o for GCC specs
#   7. GCC specs file patched (if GCC_PREFIX given)
#
# The sysroot contains NO references to solcompat in any header path.
# libsolcompat is a pure linking artifact — the sysroot IS the API.
#
# Usage:
#   make install-toolchain SYSROOT=/opt/sysroot-gcc11 [GCC_PREFIX=/opt/sst/gcc11]

SYSROOT     ?= /opt/sysroot-gcc11
GCC_PREFIX  ?=
COMPAT_BASE  = $(SYSROOT)/opt/sst/lib/solcompat

install-toolchain: libsolcompat.a
	@echo ""
	@echo "=== libsolcompat sysroot setup ==="
	@echo "  SYSROOT:     $(SYSROOT)"
	@echo "  COMPAT_BASE: $(COMPAT_BASE)"
	@echo ""
	@# --- Install sysroot-overlay (complete headers Solaris 7 lacks) ---
	@if [ -d sysroot-overlay ]; then \
		echo "Installing sysroot-overlay headers..."; \
		cp -r sysroot-overlay/* "$(SYSROOT)/"; \
		echo "  stdint.h, fenv.h, spawn.h, getopt.h, endian.h, etc."; \
	fi
	@# --- Replace sys/int_types.h entirely (fix broken 'char int8_t') ---
	@if [ -f include/sysroot-prep/sys/int_types.h.prepend ]; then \
		cp include/sysroot-prep/sys/int_types.h.prepend \
			"$(SYSROOT)/usr/include/sys/int_types.h"; \
		echo "  sys/int_types.h replaced with fixed typedefs"; \
	fi
	@# --- Prepend content to existing headers (between guard and body) ---
	@# For .prepend files (other than sys/int_types.h), insert content
	@# right after the first `#define _XXX_H` guard so declarations are
	@# visible before the original header's prototypes.
	@for prepend_file in $$(find include/sysroot-prep -name '*.prepend' -type f 2>/dev/null); do \
		rel="$${prepend_file#include/sysroot-prep/}"; \
		hdr="$${rel%.prepend}"; \
		if [ "$${hdr}" = "sys/int_types.h" ]; then continue; fi; \
		target="$(SYSROOT)/usr/include/$${hdr}"; \
		guard=$$(grep '^#ifndef _SOLCOMPAT_' "$$prepend_file" | head -1 | sed 's/#ifndef //' || true); \
		if [ -f "$${target}" ]; then \
			if [ -n "$$guard" ] && grep -q "$$guard" "$${target}" 2>/dev/null; then \
				: ; \
			else \
				awk -v content="$$(cat $$prepend_file)" ' \
					/^#define [_A-Za-z0-9]+_H/ && !inserted { print; print ""; print content; inserted=1; next } \
					{ print } \
				' "$${target}" > "$${target}.tmp" && mv "$${target}.tmp" "$${target}"; \
				echo "  Prepended $${hdr}"; \
			fi; \
		fi; \
	done
	@# --- Append declarations to existing sysroot headers ---
	@echo "Patching sysroot headers with POSIX/C99 declarations..."
	@for append_file in $$(find include/sysroot-prep -name '*.append' -type f); do \
		rel="$${append_file#include/sysroot-prep/}"; \
		hdr="$${rel%.append}"; \
		target="$(SYSROOT)/usr/include/$${hdr}"; \
		guard=$$(grep '^#ifndef _SOLCOMPAT_' "$$append_file" | head -1 | sed 's/#ifndef //'); \
		if [ -f "$${target}" ]; then \
			if [ -n "$$guard" ] && grep -q "$$guard" "$${target}" 2>/dev/null; then \
				: ; \
			else \
				echo "" >> "$${target}"; \
				cat "$$append_file" >> "$${target}"; \
				echo "  Patched $${hdr}"; \
			fi; \
		else \
			mkdir -p "$$(dirname "$${target}")"; \
			cat "$$append_file" > "$${target}"; \
			echo "  Created $${hdr}"; \
		fi; \
	done
	mkdir -p $(COMPAT_BASE)/lib
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
	@# --- Patch GCC specs if GCC_PREFIX is provided ---
	@if [ -n "$(GCC_PREFIX)" ]; then \
		echo "Patching GCC specs (library paths only)..."; \
		sh scripts/patch-specs.sh "$(GCC_PREFIX)" \
			"" \
			"$(COMPAT_BASE)/lib"; \
	fi
	@# --- Summary ---
	@echo ""
	@echo "=== Sysroot setup complete ==="
	@echo "  Sysroot headers: $(SYSROOT)/usr/include/ (GNU-standard)"
	@echo "  Augmented libm:  $(COMPAT_BASE)/lib/libm.a"
	@echo "  Augmented libsocket: $(COMPAT_BASE)/lib/libsocket.a"
	@echo "  libc supplement: $(COMPAT_BASE)/lib/libsolcompat_c.a"
	@echo "  Residual compat: $(COMPAT_BASE)/lib/libsolcompat.a"
	@echo "  values-xpg6.o:  $(COMPAT_BASE)/lib/values-xpg6.o"
	@echo ""

# ====================================================================
# install-headers — compiler-free subset of install-toolchain
# ====================================================================
# Patches the sysroot to look like a GNU-standard system WITHOUT
# requiring any .o files. Safe to run before a cross compiler exists
# (e.g. during toolchain bootstrap Phase 4).
#
# This MUST run before GCC builds so that fixincludes copies the
# already-patched headers into include-fixed/.
#
# Usage:
#   make install-headers SYSROOT=/path/to/sysroot
install-headers:
	@echo ""
	@echo "=== libsolcompat install-headers ==="
	@echo "  SYSROOT:     $(SYSROOT)"
	@echo ""
	@# --- Install sysroot-overlay (complete headers Solaris 7 lacks) ---
	@if [ -d sysroot-overlay ]; then \
		echo "Installing sysroot-overlay headers..."; \
		cp -r sysroot-overlay/* "$(SYSROOT)/"; \
		echo "  stdint.h, fenv.h, spawn.h, getopt.h, endian.h, etc."; \
	fi
	@# --- Replace sys/int_types.h entirely (fix broken 'char int8_t') ---
	@if [ -f include/sysroot-prep/sys/int_types.h.prepend ]; then \
		cp include/sysroot-prep/sys/int_types.h.prepend \
			"$(SYSROOT)/usr/include/sys/int_types.h"; \
		echo "  sys/int_types.h replaced with fixed typedefs"; \
	fi
	@# --- Prepend content to existing headers (between guard and body) ---
	@# For .prepend files (other than sys/int_types.h), insert content
	@# right after the first `#define _XXX_H` guard so declarations are
	@# visible before the original header's prototypes.
	@for prepend_file in $$(find include/sysroot-prep -name '*.prepend' -type f 2>/dev/null); do \
		rel="$${prepend_file#include/sysroot-prep/}"; \
		hdr="$${rel%.prepend}"; \
		if [ "$${hdr}" = "sys/int_types.h" ]; then continue; fi; \
		target="$(SYSROOT)/usr/include/$${hdr}"; \
		guard=$$(grep '^#ifndef _SOLCOMPAT_' "$$prepend_file" | head -1 | sed 's/#ifndef //' || true); \
		if [ -f "$${target}" ]; then \
			if [ -n "$$guard" ] && grep -q "$$guard" "$${target}" 2>/dev/null; then \
				: ; \
			else \
				awk -v content="$$(cat $$prepend_file)" ' \
					/^#define [_A-Za-z0-9]+_H/ && !inserted { print; print ""; print content; inserted=1; next } \
					{ print } \
				' "$${target}" > "$${target}.tmp" && mv "$${target}.tmp" "$${target}"; \
				echo "  Prepended $${hdr}"; \
			fi; \
		fi; \
	done
	@# --- Append declarations to existing sysroot headers ---
	@echo "Patching sysroot headers with POSIX/C99 declarations..."
	@for append_file in $$(find include/sysroot-prep -name '*.append' -type f); do \
		rel="$${append_file#include/sysroot-prep/}"; \
		hdr="$${rel%.append}"; \
		target="$(SYSROOT)/usr/include/$${hdr}"; \
		guard=$$(grep '^#ifndef _SOLCOMPAT_' "$$append_file" | head -1 | sed 's/#ifndef //'); \
		if [ -f "$${target}" ]; then \
			if [ -n "$$guard" ] && grep -q "$$guard" "$${target}" 2>/dev/null; then \
				: ; \
			else \
				echo "" >> "$${target}"; \
				cat "$$append_file" >> "$${target}"; \
				echo "  Patched $${hdr}"; \
			fi; \
		else \
			mkdir -p "$$(dirname "$${target}")"; \
			cat "$$append_file" > "$${target}"; \
			echo "  Created $${hdr}"; \
		fi; \
	done
	@echo ""
	@echo "=== install-headers complete ==="
	@echo "  Sysroot: $(SYSROOT)/usr/include/ (GNU-standard, no override paths)"
	@echo "  Run install-toolchain after cross compiler is built for library setup."
	@echo ""

# ====================================================================
# install-sysroot — Legacy: simple sysroot overlay install
# ====================================================================
install-sysroot: all
	@echo "Installing libsolcompat into sysroot..."
	mkdir -p $(SYSROOT)/usr/lib
	cp libsolcompat.a $(SYSROOT)/usr/lib/
	@echo "  libsolcompat.a installed to $(SYSROOT)/usr/lib/"

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
