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
#   make install-headers SYSROOT=    — headers only (for bootstrap / fixincludes timing)
#   make install-runtime SYSROOT=    — libsolcompat.so.1 + symlinks (target runtime)
#   make install-dev     SYSROOT=    — libsolcompat.a + headers (build-system dev)
#   make install-toolchain SYSROOT=  — install-runtime + install-dev (fat sysroot)
#
# Linkage model: libsolcompat ships as a shared library (libsolcompat.so.1)
# on Solaris targets. The .so carries DT_NEEDED for libsocket/libnsl/libresolv/
# libm/libc, so consumers get transitive deps automatically. The .a is a
# bootstrap fallback for the toolchain build and for any static-link
# scenarios on the build system; it is NOT shipped to Solaris targets.

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
       src/error.c \
       src/dl_iterate_phdr.c \
       src/complex_math.c \
       src/ssp.c

OBJS     = $(SRCS:.c=.o)
PIC_OBJS = $(SRCS:.c=.lo)

# ====================================================================
# Build targets — always produce both static and shared
# ====================================================================
all: libsolcompat.a $(SONAME)

libsolcompat.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

# Shared library with DT_NEEDED for Solaris 7 system libraries.
#
# -nodefaultlibs: suppress GCC's default LIB_SPEC (which would otherwise
# auto-append -lsolcompat, the library we're currently building).
# We explicitly list every library we need to record DT_NEEDED entries
# for so downstream consumers get transitive deps for free.
#
# -lgcc (static) only — NOT -lgcc_s. Cross-toolchains are typically built
# --disable-shared for libgcc, so libgcc_s.so.1 may not exist. libgcc.a
# carries PIC objects by default even with --disable-shared, so we can
# link it statically into the shared solcompat library.
#
# -lc must come after -lgcc because libc references some libgcc
# runtime symbols indirectly.
$(SONAME): $(PIC_OBJS)
	$(CC) -shared -Wl,-h,$(SONAME) -Wl,-z,notext -nodefaultlibs \
	      -o $@ $(PIC_OBJS) $(LDFLAGS) \
	      -lgcc -lc -lgcc -lrt -lsocket -lnsl -lresolv -lm -ldl

# Header/symbol consistency suite. Runs as part of the default build so
# CI picks it up automatically.
check: libsolcompat.a
	@CC="$(CC)" NM="$(NM)" \
	 CFLAGS="$(CFLAGS)" CPPFLAGS="$(CPPFLAGS)" \
	 LIB=libsolcompat.a \
	 sh tests/check_headers.sh

# Compile rules
.SUFFIXES: .c .o .lo

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.c.lo:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PICFLAGS) -c -o $@ $<

# math.o needs -fno-builtin for the entire TU.  math.c implements roughly
# 50 C99 math functions that are missing from Solaris 7 libm.  Every
# `float Xf(float x) { return (float)X((double)x); }` wrapper is a trap
# for GCC's builtin recognizer: GCC will see the idiom and fold it into
# a direct `Xf(x)` call — which tail-recurses forever because we ARE
# Xf's definition.  Observed in the wild: roundf at -O2 compiled to a
# bare `call roundf@plt` that loops forever under LD_PRELOAD ordering.
# Also covers the earlier fma/fmaf/fmal conflict with GCC 15's implicit
# inline declarations.
src/math.o: src/math.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -fno-builtin -c -o $@ $<
src/math.lo: src/math.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PICFLAGS) -fno-builtin -c -o $@ $<

# complex_math.o hits the same builtin-fold trap: our cexp/cexpf bodies
# look like `expf(re)*cosf(im) + i*expf(re)*sinf(im)`, which GCC will
# collapse back into a cexp/cexpf call → self-recursion.
src/complex_math.o: src/complex_math.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -fno-builtin -c -o $@ $<
src/complex_math.lo: src/complex_math.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PICFLAGS) -fno-builtin -c -o $@ $<

# ====================================================================
# install-headers — patch sysroot to look GNU-standard
# ====================================================================
# Compiler-free; safe to run before a cross compiler exists (e.g.
# during toolchain bootstrap before GCC build so fixincludes captures
# the already-patched headers into include-fixed/).
#
# Three mechanisms:
#   1. sysroot-overlay/     — complete headers Solaris 7 lacks
#                             (stdint.h, fenv.h, spawn.h, getopt.h,
#                             endian.h, complex.h, stdbool.h, ...)
#   2. sysroot-prep/*.prepend — inserted after the first #define _XXX_H
#                               guard of an existing system header
#   3. sysroot-prep/*.append  — appended to existing system headers
#
# The sysroot contains NO references to solcompat/ in any header path.
# libsolcompat is a pure linking artifact — the sysroot IS the API.
install-headers:
	@echo ""
	@echo "=== libsolcompat install-headers ==="
	@echo "  SYSROOT:     $(SYSROOT)"
	@echo ""
	@# --- Install sysroot-overlay (complete headers Solaris 7 lacks,
	@#     plus wholesale replacements like sys/int_types.h) ---
	@if [ -d sysroot-overlay ]; then \
		echo "Installing sysroot-overlay headers..."; \
		cp -r sysroot-overlay/* "$(SYSROOT)/"; \
		echo "  stdint.h, fenv.h, spawn.h, getopt.h, endian.h,"; \
		echo "  sys/int_types.h (fixed typedefs), etc."; \
	fi
	@# --- Patch sys/types.h: make longlong_t a real 'long long' always ---
	@# Solaris 7's sys/types.h gates the 'typedef long long longlong_t'
	@# on '__STDC__ - 0 == 0' (K&R-era compilers) and
	@# '!defined(__STRICT_ANSI__)'.  Both fail for modern C compilers
	@# in strict mode:
	@#   - gcc -std=c99 / -std=c11 sets __STDC__=1 AND __STRICT_ANSI__
	@#   - gcc -std=gnu99 sets __STDC__=1 but not __STRICT_ANSI__
	@#   - C++ compilers set __STDC__=1 (or undef it)
	@#
	@# When both gates fail, longlong_t becomes a union — which breaks
	@# off_t (= longlong_t with _FILE_OFFSET_BITS=64).  Any code that
	@# casts an integer to off_t (e.g. `(off_t)-1`, `(off_t)lseek(...)`)
	@# then fails with "cast to union type from type not present in
	@# union".
	@#
	@# GCC 15 unconditionally supports `long long` (C99 and C++11+),
	@# so the gate is obsolete.  Force-unconditional `long long`
	@# typedef by rewriting both gates to always evaluate true.
	@# IDEMPOTENT migration — handle FOUR pattern variants:
	@#   1. Raw Solaris 7:     #if __STDC__ - 0 == 0 && !defined(_NO_LONGLONG)
	@#   2. Raw Solaris 7 alt: #if  !defined(__STRICT_ANSI__) && !defined(_NO_LONGLONG)
	@#   3. Prior patch (C++): #if (defined(__cplusplus) || __STDC__ - 0 == 0) && !defined(_NO_LONGLONG)
	@#   4. Prior patch alt:   #if (defined(__cplusplus) || !defined(__STRICT_ANSI__)) && !defined(_NO_LONGLONG)
	@# All rewrite to the unconditional: #if !defined(_NO_LONGLONG)
	@# This handles persistent sysroot mounts that have accumulated
	@# prior patches across runs.
	@for longlong_header in \
	    "$(SYSROOT)/usr/include/sys/types.h" \
	    "$(SYSROOT)/usr/include/stdio.h"; do \
		[ -f "$$longlong_header" ] || continue; \
		awk '/^#if __STDC__ - 0 == 0 && !defined\(_NO_LONGLONG\)$$/ { \
		        print "/* _SOLCOMPAT_LONGLONG_CXX */"; \
		        print "#if !defined(_NO_LONGLONG)"; \
		        next; \
		     } \
		     /^#if  !defined\(__STRICT_ANSI__\) && !defined\(_NO_LONGLONG\)$$/ { \
		        print "/* _SOLCOMPAT_LONGLONG_CXX */"; \
		        print "#if !defined(_NO_LONGLONG)"; \
		        next; \
		     } \
		     /^#if \(defined\(__cplusplus\) \|\| __STDC__ - 0 == 0\) && !defined\(_NO_LONGLONG\)$$/ { \
		        print "#if !defined(_NO_LONGLONG)"; \
		        next; \
		     } \
		     /^#if \(defined\(__cplusplus\) \|\| !defined\(__STRICT_ANSI__\)\) && !defined\(_NO_LONGLONG\)$$/ { \
		        print "#if !defined(_NO_LONGLONG)"; \
		        next; \
		     } \
		     { print }' "$$longlong_header" > "$$longlong_header.tmp" && \
		mv "$$longlong_header.tmp" "$$longlong_header"; \
		if ! grep -q '^#if !defined(_NO_LONGLONG)$$' "$$longlong_header"; then \
			echo "FATAL: longlong_t unconditional form not present in $$longlong_header after awk" >&2; \
			grep -n '_NO_LONGLONG\|__STDC__\|__STRICT_ANSI__' "$$longlong_header" | head -10 >&2; \
			exit 1; \
		fi; \
		echo "  $$longlong_header longlong_t unconditional (always long long)" >&2; \
	done
	@# Path resolution rule for sysroot-prep entries:
	@#   If the relative path under sysroot-prep starts with "usr/" or
	@#   "opt/" we treat it as absolute-from-SYSROOT (so X11/openwin and
	@#   future SST-prefix headers can be patched directly). Otherwise
	@#   the entry is anchored at $(SYSROOT)/usr/include/<rel> for
	@#   backwards compatibility with the existing sys/socket.h, etc.
	@# --- Prepend content to existing headers (between guard and body) ---
	@for prepend_file in $$(find include/sysroot-prep -name '*.prepend' -type f 2>/dev/null); do \
		rel="$${prepend_file#include/sysroot-prep/}"; \
		hdr="$${rel%.prepend}"; \
		case "$${hdr}" in \
			usr/*|opt/*) target="$(SYSROOT)/$${hdr}" ;; \
			*)           target="$(SYSROOT)/usr/include/$${hdr}" ;; \
		esac; \
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
	@# If the target already contains our guard, strip the existing
	@# _SOLCOMPAT_*_APPEND block first so newer .append content
	@# (added macros / declarations) replaces the old block in place.
	@# A plain guard-present skip leaves stale content forever: any
	@# SYSROOT_ORIG that was patched by an earlier install-headers run
	@# would permanently shadow .append updates, so libsolcompat bumps
	@# would silently no-op at sysroot-build time.
	@echo "Patching sysroot headers with POSIX/C99 declarations..."
	@for append_file in $$(find include/sysroot-prep -name '*.append' -type f); do \
		rel="$${append_file#include/sysroot-prep/}"; \
		hdr="$${rel%.append}"; \
		case "$${hdr}" in \
			usr/*|opt/*) target="$(SYSROOT)/$${hdr}" ;; \
			*)           target="$(SYSROOT)/usr/include/$${hdr}" ;; \
		esac; \
		guard=$$(grep '^#ifndef _SOLCOMPAT_' "$$append_file" | head -1 | sed 's/#ifndef //'); \
		if [ -f "$${target}" ]; then \
			if [ -n "$$guard" ] && grep -q "^#ifndef $$guard$$" "$${target}" 2>/dev/null; then \
				sed -i "/^#ifndef $$guard$$/,/^#endif.*$$guard/d" "$${target}"; \
			fi; \
			echo "" >> "$${target}"; \
			cat "$$append_file" >> "$${target}"; \
			echo "  Patched $${hdr}"; \
		else \
			mkdir -p "$$(dirname "$${target}")"; \
			cat "$$append_file" > "$${target}"; \
			echo "  Created $${hdr}"; \
		fi; \
	done

# ====================================================================
# install-runtime — shared library only (what ships to Solaris target)
# ====================================================================
install-runtime: $(SONAME)
	@echo ""
	@echo "=== libsolcompat install-runtime ==="
	@echo "  SYSROOT: $(SYSROOT)"
	mkdir -p "$(SYSROOT)/usr/lib"
	cp $(SONAME) "$(SYSROOT)/usr/lib/$(SONAME)"
	ln -sf $(SONAME) "$(SYSROOT)/usr/lib/libsolcompat.so"
	@echo "  $(SONAME) + libsolcompat.so symlink"

# ====================================================================
# install-dev — static archive + patched headers (build-system only)
# ====================================================================
install-dev: libsolcompat.a install-headers
	@echo ""
	@echo "=== libsolcompat install-dev ==="
	@echo "  SYSROOT: $(SYSROOT)"
	mkdir -p "$(SYSROOT)/usr/lib"
	cp libsolcompat.a "$(SYSROOT)/usr/lib/libsolcompat.a"
	@echo "  libsolcompat.a installed"

# ====================================================================
# install-toolchain — convenience: runtime + dev (fat sysroot)
# ====================================================================
# Used during toolchain bootstrap to populate the cross-build sysroot
# with everything needed to compile AND link downstream packages.
install-toolchain: install-runtime install-dev

# ====================================================================
# install — traditional PREFIX-based install (for manual/native builds)
# ====================================================================
install: all
	mkdir -p $(DESTDIR)$(LIBDIR)
	mkdir -p $(DESTDIR)$(DOCDIR)
	cp libsolcompat.a $(DESTDIR)$(LIBDIR)/
	cp $(SONAME) $(DESTDIR)$(LIBDIR)/
	ln -sf $(SONAME) $(DESTDIR)$(LIBDIR)/libsolcompat.so
	cp README.md $(DESTDIR)$(DOCDIR)/ 2>/dev/null || true
	cp LICENSE $(DESTDIR)$(DOCDIR)/ 2>/dev/null || true

# ====================================================================
# Tests (run on Solaris target or under QEMU)
# ====================================================================
# Links against libsolcompat.a with explicit transitive deps because
# test_all is invoked from the build dir where no libsolcompat.so
# symlink exists; this exercises the static archive path.
test: all
	cd tests && $(CC) $(CPPFLAGS) $(CFLAGS) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 \
		-o test_all test_all.c \
		-L$(CURDIR) -lsolcompat -lrt -lsocket -lnsl -lresolv -lm -ldl && ./test_all

# ====================================================================
# Clean
# ====================================================================
clean:
	rm -f $(OBJS) $(PIC_OBJS) libsolcompat.a $(SONAME)
	rm -f tests/test_all
	rm -rf tests/gen

.PHONY: all check install install-headers install-runtime install-dev install-toolchain test clean
# cache-bust: 2026-04-17 — force sysroot rebuild after OCI cache poisoning with awk patch
