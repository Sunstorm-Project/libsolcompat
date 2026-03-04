/*
 * solcompat/solcompat.h — Master include for libsolcompat
 *
 * POSIX/C99/C11 compatibility library for Solaris 7 SPARC.
 * Include this header (or compile with -include solcompat/solcompat.h)
 * to get all compatibility shims.
 *
 * Copyright (c) 2025 Julian Wolfe
 * MIT License — see LICENSE
 */
#ifndef SOLCOMPAT_H
#define SOLCOMPAT_H

#define SOLCOMPAT_VERSION_MAJOR 0
#define SOLCOMPAT_VERSION_MINOR 2
#define SOLCOMPAT_VERSION_PATCH 0
#define SOLCOMPAT_VERSION_STRING "0.2.0"

/* Pull in all subsystem headers */
#include <solcompat/snprintf.h>
#include <solcompat/string_ext.h>
#include <solcompat/stdio_ext.h>
#include <solcompat/stdlib_ext.h>
#include <solcompat/c99_types.h>
#include <solcompat/network.h>
#include <solcompat/clock.h>
#include <solcompat/math_ext.h>
#include <solcompat/memory.h>
#include <solcompat/filesystem.h>
#include <solcompat/at_funcs.h>
#include <solcompat/pty.h>
#include <solcompat/process.h>
#include <solcompat/poll_ext.h>
#include <solcompat/random.h>
#include <solcompat/stubs.h>
#include <solcompat/kstat_ext.h>
#include <solcompat/hwcap.h>
#include <solcompat/xpg.h>
#include <solcompat/compat_types.h>

#endif /* SOLCOMPAT_H */
