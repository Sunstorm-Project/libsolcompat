/*
 * Part of libsolcompat sysroot — provides Solaris 10+ atomic operations for Solaris 7
 *
 * Solaris 10+ provides <atomic.h> which includes <sys/atomic.h>.
 * Solaris 7 only has <sys/atomic.h> directly.  This header provides
 * the expected <atomic.h> include path plus convenience wrappers
 * implemented in libsolcompat.
 */
#ifndef _SOLCOMPAT_SYSROOT_ATOMIC_H
#define _SOLCOMPAT_SYSROOT_ATOMIC_H

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

#endif /* _SOLCOMPAT_SYSROOT_ATOMIC_H */
