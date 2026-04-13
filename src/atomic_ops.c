/*
 * atomic_ops.c — GCC __atomic built-in implementations for SPARCv7/v8
 *
 * GCC's __atomic_fetch_*() built-in functions are emitted as library
 * calls on targets without hardware atomic instructions (SPARCv7/v8
 * with -mcpu=v7).  Normally libatomic provides these, but our cross-
 * compiler was built without libatomic.
 *
 * On Solaris 7, cas32()/cas64() from <sys/atomic.h> are kernel-only
 * functions NOT available in userland.  We provide our own portable
 * implementations suitable for single-CPU systems (e.g. QEMU).
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */

#include <sys/types.h>

/* ================================================================
 * Solaris atomic primitives (cas32/cas64/membar_*)
 *
 * These are declared in <sys/atomic.h> but on Solaris 7 they only
 * exist in the kernel (genunix).  Provide userland implementations
 * for single-CPU systems.  On multi-CPU this would need hardware
 * CAS or a spinlock, but Solaris 7 on QEMU is single-CPU.
 * ================================================================ */

uint32_t
cas32(uint32_t *target, uint32_t cmp, uint32_t newval)
{
	uint32_t old = *target;
	if (old == cmp)
		*target = newval;
	return old;
}

uint64_t
cas64(uint64_t *target, uint64_t cmp, uint64_t newval)
{
	uint64_t old = *target;
	if (old == cmp)
		*target = newval;
	return old;
}

ulong_t
caslong(ulong_t *target, ulong_t cmp, ulong_t newval)
{
	ulong_t old = *target;
	if (old == cmp)
		*target = newval;
	return old;
}

void *
casptr(void *target, void *cmp, void *newval)
{
	void **p = (void **)target;
	void *old = *p;
	if (old == cmp)
		*p = newval;
	return old;
}

void atomic_add_16(uint16_t *target, int16_t delta) { *target += delta; }
void atomic_add_32(uint32_t *target, int32_t delta) { *target += delta; }
void atomic_add_long(ulong_t *target, long delta)   { *target += delta; }
void atomic_add_64(uint64_t *target, int64_t delta)  { *target += delta; }

uint16_t atomic_add_16_nv(uint16_t *t, int16_t d) { return (*t += d); }
uint32_t atomic_add_32_nv(uint32_t *t, int32_t d) { return (*t += d); }
ulong_t  atomic_add_long_nv(ulong_t *t, long d)   { return (*t += d); }
uint64_t atomic_add_64_nv(uint64_t *t, int64_t d) { return (*t += d); }

void membar_enter(void)    { /* no-op on single-CPU */ }
void membar_exit(void)     { /* no-op on single-CPU */ }
void membar_producer(void) { /* no-op on single-CPU */ }
void membar_consumer(void) { /* no-op on single-CPU */ }

/* ================================================================
 * Solaris 10+ convenience wrappers
 *
 * These functions exist in Solaris 10+ but not Solaris 7.
 * They are thin wrappers around the primitives above.
 * SDL2 2.32.10's Solaris codepath uses these directly.
 * ================================================================ */

uint_t
atomic_cas_uint(volatile uint_t *target, uint_t cmp, uint_t newval)
{
	return (uint_t)cas32((uint32_t *)target, (uint32_t)cmp,
	    (uint32_t)newval);
}

void *
atomic_cas_ptr(volatile void *target, void *cmp, void *newval)
{
	return casptr((void *)target, cmp, newval);
}

uint_t
atomic_swap_uint(volatile uint_t *target, uint_t newval)
{
	uint_t old_value;
	do {
		old_value = *target;
	} while (atomic_cas_uint(target, old_value, newval) != old_value);
	return old_value;
}

void *
atomic_swap_ptr(volatile void *target, void *newval)
{
	void *old_value;
	do {
		old_value = *(void *volatile *)target;
	} while (atomic_cas_ptr(target, old_value, newval) != old_value);
	return old_value;
}

void
atomic_add_int(volatile uint_t *target, int delta)
{
	atomic_add_32((uint32_t *)target, (int32_t)delta);
}

uint_t
atomic_or_uint(volatile uint_t *target, uint_t bits)
{
	uint_t old_value;
	do {
		old_value = *target;
	} while (atomic_cas_uint(target, old_value, old_value | bits)
	    != old_value);
	return old_value;
}

/* ================================================================
 * Utility
 * ================================================================ */

_Bool __atomic_is_lock_free(unsigned int size, const volatile void *ptr)
{
	(void)ptr;
	return (size <= 8) ? 1 : 0;
}

/* ================================================================
 * 1-byte atomic operations (use CAS on containing 32-bit word)
 * On single-CPU SPARC, simple load/store is sufficient.
 * ================================================================ */

_Bool
__atomic_compare_exchange_1(volatile void *ptr, void *expected,
    unsigned char desired, _Bool weak, int success_memorder,
    int failure_memorder)
{
	volatile unsigned char *p = (volatile unsigned char *)ptr;
	unsigned char *e = (unsigned char *)expected;
	if (*p == *e) {
		*p = desired;
		return 1;
	}
	*e = *p;
	return 0;
}

unsigned char
__atomic_exchange_1(volatile void *ptr, unsigned char val, int memorder)
{
	volatile unsigned char *target_ptr = (volatile unsigned char *)ptr;
	unsigned char old_value = *target_ptr;
	(void)memorder;
	*target_ptr = val;
	return old_value;
}

unsigned char
__atomic_fetch_add_1(volatile void *ptr, unsigned char val, int memorder)
{
	volatile unsigned char *target_ptr = (volatile unsigned char *)ptr;
	unsigned char old_value = *target_ptr;
	(void)memorder;
	*target_ptr = old_value + val;
	return old_value;
}

unsigned char
__atomic_fetch_sub_1(volatile void *ptr, unsigned char val, int memorder)
{
	volatile unsigned char *target_ptr = (volatile unsigned char *)ptr;
	unsigned char old_value = *target_ptr;
	(void)memorder;
	*target_ptr = old_value - val;
	return old_value;
}

unsigned char
__atomic_fetch_and_1(volatile void *ptr, unsigned char val, int memorder)
{
	volatile unsigned char *target_ptr = (volatile unsigned char *)ptr;
	unsigned char old_value = *target_ptr;
	(void)memorder;
	*target_ptr = old_value & val;
	return old_value;
}

unsigned char
__atomic_fetch_or_1(volatile void *ptr, unsigned char val, int memorder)
{
	volatile unsigned char *target_ptr = (volatile unsigned char *)ptr;
	unsigned char old_value = *target_ptr;
	(void)memorder;
	*target_ptr = old_value | val;
	return old_value;
}

unsigned char
__atomic_fetch_xor_1(volatile void *ptr, unsigned char val, int memorder)
{
	volatile unsigned char *target_ptr = (volatile unsigned char *)ptr;
	unsigned char old_value = *target_ptr;
	(void)memorder;
	*target_ptr = old_value ^ val;
	return old_value;
}

/* ================================================================
 * 2-byte (uint16_t) atomic operations
 * On single-CPU SPARC, simple load/store is sufficient.
 * ================================================================ */

_Bool
__atomic_compare_exchange_2(volatile void *ptr, void *expected,
    unsigned short desired, _Bool weak, int success_memorder,
    int failure_memorder)
{
	volatile unsigned short *target_ptr = (volatile unsigned short *)ptr;
	unsigned short *expected_ptr = (unsigned short *)expected;
	(void)weak; (void)success_memorder; (void)failure_memorder;
	if (*target_ptr == *expected_ptr) {
		*target_ptr = desired;
		return 1;
	}
	*expected_ptr = *target_ptr;
	return 0;
}

unsigned short
__atomic_exchange_2(volatile void *ptr, unsigned short val, int memorder)
{
	volatile unsigned short *target_ptr = (volatile unsigned short *)ptr;
	unsigned short old_value = *target_ptr;
	(void)memorder;
	*target_ptr = val;
	return old_value;
}

unsigned short
__atomic_fetch_add_2(volatile void *ptr, unsigned short val, int memorder)
{
	volatile unsigned short *target_ptr = (volatile unsigned short *)ptr;
	unsigned short old_value = *target_ptr;
	(void)memorder;
	*target_ptr = old_value + val;
	return old_value;
}

unsigned short
__atomic_fetch_sub_2(volatile void *ptr, unsigned short val, int memorder)
{
	volatile unsigned short *target_ptr = (volatile unsigned short *)ptr;
	unsigned short old_value = *target_ptr;
	(void)memorder;
	*target_ptr = old_value - val;
	return old_value;
}

unsigned short
__atomic_fetch_and_2(volatile void *ptr, unsigned short val, int memorder)
{
	volatile unsigned short *target_ptr = (volatile unsigned short *)ptr;
	unsigned short old_value = *target_ptr;
	(void)memorder;
	*target_ptr = old_value & val;
	return old_value;
}

unsigned short
__atomic_fetch_or_2(volatile void *ptr, unsigned short val, int memorder)
{
	volatile unsigned short *target_ptr = (volatile unsigned short *)ptr;
	unsigned short old_value = *target_ptr;
	(void)memorder;
	*target_ptr = old_value | val;
	return old_value;
}

unsigned short
__atomic_fetch_xor_2(volatile void *ptr, unsigned short val, int memorder)
{
	volatile unsigned short *target_ptr = (volatile unsigned short *)ptr;
	unsigned short old_value = *target_ptr;
	(void)memorder;
	*target_ptr = old_value ^ val;
	return old_value;
}

/* ================================================================
 * 4-byte (uint32_t) atomic operations using cas32()
 * ================================================================ */

uint32_t
__atomic_fetch_add_4(volatile void *ptr, uint32_t val, int memorder)
{
	uint32_t old;
	do {
		old = *(volatile uint32_t *)ptr;
	} while (cas32((uint32_t *)ptr, old, old + val) != old);
	return old;
}

uint32_t
__atomic_fetch_sub_4(volatile void *ptr, uint32_t val, int memorder)
{
	uint32_t old;
	do {
		old = *(volatile uint32_t *)ptr;
	} while (cas32((uint32_t *)ptr, old, old - val) != old);
	return old;
}

uint32_t
__atomic_fetch_and_4(volatile void *ptr, uint32_t val, int memorder)
{
	uint32_t old;
	do {
		old = *(volatile uint32_t *)ptr;
	} while (cas32((uint32_t *)ptr, old, old & val) != old);
	return old;
}

uint32_t
__atomic_fetch_or_4(volatile void *ptr, uint32_t val, int memorder)
{
	uint32_t old;
	do {
		old = *(volatile uint32_t *)ptr;
	} while (cas32((uint32_t *)ptr, old, old | val) != old);
	return old;
}

uint32_t
__atomic_fetch_xor_4(volatile void *ptr, uint32_t val, int memorder)
{
	uint32_t old;
	do {
		old = *(volatile uint32_t *)ptr;
	} while (cas32((uint32_t *)ptr, old, old ^ val) != old);
	return old;
}

uint32_t
__atomic_exchange_4(volatile void *ptr, uint32_t val, int memorder)
{
	uint32_t old;
	do {
		old = *(volatile uint32_t *)ptr;
	} while (cas32((uint32_t *)ptr, old, val) != old);
	return old;
}

_Bool
__atomic_compare_exchange_4(volatile void *ptr, void *expected,
    uint32_t desired, _Bool weak, int success_memorder, int failure_memorder)
{
	uint32_t exp = *(uint32_t *)expected;
	uint32_t old = cas32((uint32_t *)ptr, exp, desired);
	if (old == exp)
		return 1;
	*(uint32_t *)expected = old;
	return 0;
}

/* ================================================================
 * 8-byte (uint64_t) atomic operations using cas64()
 * ================================================================ */

uint64_t
__atomic_fetch_add_8(volatile void *ptr, uint64_t val, int memorder)
{
	uint64_t old;
	do {
		old = *(volatile uint64_t *)ptr;
	} while (cas64((uint64_t *)ptr, old, old + val) != old);
	return old;
}

uint64_t
__atomic_fetch_sub_8(volatile void *ptr, uint64_t val, int memorder)
{
	uint64_t old;
	do {
		old = *(volatile uint64_t *)ptr;
	} while (cas64((uint64_t *)ptr, old, old - val) != old);
	return old;
}

uint64_t
__atomic_fetch_and_8(volatile void *ptr, uint64_t val, int memorder)
{
	uint64_t old;
	do {
		old = *(volatile uint64_t *)ptr;
	} while (cas64((uint64_t *)ptr, old, old & val) != old);
	return old;
}

uint64_t
__atomic_fetch_or_8(volatile void *ptr, uint64_t val, int memorder)
{
	uint64_t old;
	do {
		old = *(volatile uint64_t *)ptr;
	} while (cas64((uint64_t *)ptr, old, old | val) != old);
	return old;
}

uint64_t
__atomic_fetch_xor_8(volatile void *ptr, uint64_t val, int memorder)
{
	uint64_t old;
	do {
		old = *(volatile uint64_t *)ptr;
	} while (cas64((uint64_t *)ptr, old, old ^ val) != old);
	return old;
}

uint64_t
__atomic_exchange_8(volatile void *ptr, uint64_t val, int memorder)
{
	uint64_t old;
	do {
		old = *(volatile uint64_t *)ptr;
	} while (cas64((uint64_t *)ptr, old, val) != old);
	return old;
}

_Bool
__atomic_compare_exchange_8(volatile void *ptr, void *expected,
    uint64_t desired, _Bool weak, int success_memorder, int failure_memorder)
{
	uint64_t exp = *(uint64_t *)expected;
	uint64_t old = cas64((uint64_t *)ptr, exp, desired);
	if (old == exp)
		return 1;
	*(uint64_t *)expected = old;
	return 0;
}

/* ================================================================
 * Load/Store for 8-byte values (needed on 32-bit targets where
 * 64-bit loads/stores aren't naturally atomic)
 * ================================================================ */

uint64_t
__atomic_load_8(const volatile void *ptr, int memorder)
{
	/*
	 * Use cas64 with same old/new value to atomically read.
	 * This is a common trick: CAS that doesn't change the value
	 * but returns the current value atomically.
	 */
	uint64_t val = *(const volatile uint64_t *)ptr;
	return cas64((uint64_t *)ptr, val, val);
}

void
__atomic_store_8(volatile void *ptr, uint64_t val, int memorder)
{
	uint64_t old;
	do {
		old = *(volatile uint64_t *)ptr;
	} while (cas64((uint64_t *)ptr, old, val) != old);
}

/* ================================================================
 * 4-byte load/store (naturally atomic on SPARC for aligned words)
 * ================================================================ */

uint32_t
__atomic_load_4(const volatile void *ptr, int memorder)
{
	return *(const volatile uint32_t *)ptr;
}

void
__atomic_store_4(volatile void *ptr, uint32_t val, int memorder)
{
	*(volatile uint32_t *)ptr = val;
}

/* ================================================================
 * Nand (needed by some code, ~(old & val))
 * ================================================================ */

uint64_t
__atomic_fetch_nand_8(volatile void *ptr, uint64_t val, int memorder)
{
	uint64_t old;
	do {
		old = *(volatile uint64_t *)ptr;
	} while (cas64((uint64_t *)ptr, old, ~(old & val)) != old);
	return old;
}
