/*
 * override/atomic.h — Wrapper for Solaris atomic operations
 *
 * Solaris 10+ provides <atomic.h> which simply includes <sys/atomic.h>.
 * Solaris 7/8/9 only have <sys/atomic.h> directly.  This override
 * provides the expected <atomic.h> include path so that software
 * written for Solaris 10+ (e.g. OpenSSL 3.x) can find it.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_ATOMIC_H
#define _SOLCOMPAT_OVERRIDE_ATOMIC_H

/*
 * __sun guard: <sys/atomic.h> and uint_t are Solaris-specific.  On
 * glibc hosts (canadian-cross build-side x86 compiles), this override
 * path may leak via -isystem.  Make it a no-op elsewhere so a stray
 * -isystem search hit doesn't fail trying to find a Solaris header
 * that isn't there.
 */
#ifdef __sun

#include <sys/atomic.h>

/*
 * Solaris 10+ convenience wrappers — not present in Solaris 7.
 * Provided by libsolcompat's atomic_ops.c.
 */
#ifdef __cplusplus
extern "C" {
#endif

extern uint_t atomic_cas_uint(volatile uint_t *, uint_t, uint_t);
extern void  *atomic_cas_ptr(volatile void *, void *, void *);
extern uint_t atomic_swap_uint(volatile uint_t *, uint_t);
extern void  *atomic_swap_ptr(volatile void *, void *);
extern void   atomic_add_int(volatile uint_t *, int);
extern uint_t atomic_or_uint(volatile uint_t *, uint_t);

#ifdef __cplusplus
}
#endif

#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_ATOMIC_H */
