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
       src/atomic_ops.c

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

# Shared library — only built on request or when linking works
$(SONAME): $(PIC_OBJS)
	$(CC) -shared -Wl,-h,$(SONAME) -Wl,-z,notext -o $@ $(PIC_OBJS) $(LDFLAGS) -lrt -lsocket -lnsl -lresolv -lm -ldl

# Compile rules
.SUFFIXES: .c .o .lo

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.c.lo:
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PICFLAGS) -c -o $@ $<

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
#   1. Override headers installed to SYSROOT/opt/sst/include/override/
#      (GCC specs will add -isystem pointing there)
#   2. solcompat/ internal headers alongside override headers
#   3. Math objects merged into a COPY of libm.a in the override lib dir
#   4. Network objects merged into the override lib dir as libsocket additions
#   5. General POSIX objects merged into override libc additions
#   6. Residual libsolcompat.a for misc stubs (auto-linked via specs)
#   7. values-xpg6.o extracted for GCC specs
#   8. GCC specs file patched (if GCC_PREFIX given)
#
# After this, AC_CHECK_FUNC(fabsf) finds it in augmented libm.a,
# AC_CHECK_HEADER(stdint.h) finds it in override dir, and -lsolcompat
# is auto-linked into every binary.
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

install-toolchain: all
	@echo ""
	@echo "=== libsolcompat scatter-install ==="
	@echo "  SYSROOT:     $(SYSROOT)"
	@echo "  COMPAT_BASE: $(COMPAT_BASE)"
	@echo ""
	@# --- Create directory structure ---
	mkdir -p $(COMPAT_BASE)/include/override/sys
	mkdir -p $(COMPAT_BASE)/include/override/arpa
	mkdir -p $(COMPAT_BASE)/include/override/net
	mkdir -p $(COMPAT_BASE)/include/override/netinet
	mkdir -p $(COMPAT_BASE)/include/solcompat
	mkdir -p $(COMPAT_BASE)/lib
	@# --- Install override wrapper headers ---
	@echo "Installing override headers..."
	cp include/override/*.h $(COMPAT_BASE)/include/override/
	cp include/override/sys/*.h $(COMPAT_BASE)/include/override/sys/
	cp include/override/arpa/*.h $(COMPAT_BASE)/include/override/arpa/
	cp include/override/net/*.h $(COMPAT_BASE)/include/override/net/
	cp include/override/netinet/*.h $(COMPAT_BASE)/include/override/netinet/
	@echo "  override/math.h, stdint.h, netdb.h, sys/socket.h, netinet/in.h, arpa/inet.h, etc."
	@# --- Install solcompat internal headers ---
	@echo "Installing solcompat headers..."
	cp include/solcompat/*.h $(COMPAT_BASE)/include/solcompat/
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
	@if [ -n "$(GCC_PREFIX)" ]; then \
		echo "Patching GCC specs..."; \
		sh scripts/patch-specs.sh "$(GCC_PREFIX)" \
			"$(COMPAT_BASE)/include/override" \
			"$(COMPAT_BASE)/lib"; \
	else \
		echo ""; \
		echo "GCC_PREFIX not set — skipping specs patching."; \
		echo "For cross-build, add these flags manually or patch specs later:"; \
		echo "  CFLAGS:  -isystem $(COMPAT_BASE)/include/override -isystem $(COMPAT_BASE)/include"; \
		echo "  LDFLAGS: -L$(COMPAT_BASE)/lib -lsolcompat -lsolcompat_c"; \
	fi
	@# --- Summary ---
	@echo ""
	@echo "=== Scatter-install complete ==="
	@echo "  Override headers: $(COMPAT_BASE)/include/override/"
	@echo "  Augmented libm:  $(COMPAT_BASE)/lib/libm.a"
	@echo "  Augmented libsocket: $(COMPAT_BASE)/lib/libsocket.a"
	@echo "  libc supplement: $(COMPAT_BASE)/lib/libsolcompat_c.a"
	@echo "  Residual compat: $(COMPAT_BASE)/lib/libsolcompat.a"
	@echo "  values-xpg6.o:  $(COMPAT_BASE)/lib/values-xpg6.o"
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

.PHONY: all install install-toolchain install-sysroot test clean
