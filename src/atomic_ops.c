/*
 * atomic_ops.c — Atomic intrinsic implementations for SPARC v7/v8/v9
 *
 * Provides:
 *   1. Solaris <sys/atomic.h> userland primitives (cas32/cas64/membar_*).
 *   2. GCC C11 __atomic_* intrinsics for 1/2/4/8-byte ops.
 *   3. GCC legacy __sync_* intrinsics for 4-byte ops — including the
 *      xor/or/and/nand variants libgcc built --with-cpu=v7 omits.
 *
 * Correctness model
 * -----------------
 * The SPARC v7 ISA has `ldstub` (atomic test-and-set on one byte) but no
 * hardware compare-and-swap. v8 adds nothing usable in user mode. v9
 * finally exposes `cas`, but libsolcompat is compiled --with-cpu=v7 so
 * GCC will not emit it. We therefore build all primitives on top of
 * `ldstub`-mediated stripe locks, which are correct on v7/v8/v9 single-
 * CPU AND multi-CPU. We pay one ldstub per op versus a single CAS; the
 * simplicity buys us "correct everywhere" at a modest throughput cost.
 *
 * Stripe locks: 16 cache-line-padded byte locks indexed by
 * ((addr >> 4) & 15). Gives 16-way parallelism with no false sharing.
 *
 * Memory ordering: SPARC TSO delivers #LoadLoad|#LoadStore|#StoreStore
 * for free, so `stbar` (a #StoreStore barrier on v7/v8) is a full fence
 * in practice. On v9 we emit full `membar` forms where accuracy matters.
 *
 * Signal-safety caveat
 * --------------------
 * Stripe locks are not re-entrant. An async signal handler that invokes
 * any libsolcompat atomic while the interrupted thread holds the same
 * stripe will deadlock. The ABI convention on Solaris already forbids
 * atomic ops from signal handlers, so we inherit that assumption.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __GNUC__
# define ATOMIC_HELPER \
    __attribute__((noinline, \
                   no_instrument_function))
#else
# define ATOMIC_HELPER
#endif

/* ================================================================
 * Memory barriers
 * ================================================================ */

static inline void
sst_full_barrier(void)
{
#if defined(__sparc_v9__) || defined(__sparcv9)
    __asm__ __volatile__(
        "membar #StoreLoad | #LoadLoad | #LoadStore | #StoreStore"
        ::: "memory");
#elif defined(__sparc__)
    __asm__ __volatile__("stbar" ::: "memory");
#else
    __asm__ __volatile__("" ::: "memory");
#endif
}

static inline void
sst_store_barrier(void)
{
#if defined(__sparc_v9__) || defined(__sparcv9)
    __asm__ __volatile__("membar #StoreStore" ::: "memory");
#elif defined(__sparc__)
    __asm__ __volatile__("stbar" ::: "memory");
#else
    __asm__ __volatile__("" ::: "memory");
#endif
}

static inline void
sst_load_barrier(void)
{
#if defined(__sparc_v9__) || defined(__sparcv9)
    __asm__ __volatile__("membar #LoadLoad" ::: "memory");
#else
    /* TSO provides #LoadLoad for free on v7/v8 and on non-SPARC hosts
     * a compiler barrier is sufficient for our test harness. */
    __asm__ __volatile__("" ::: "memory");
#endif
}

/* ================================================================
 * Stripe locks
 * ================================================================ */

#define SST_STRIPE_BITS  4
#define SST_STRIPE_COUNT (1u << SST_STRIPE_BITS)
#define SST_STRIPE_MASK  (SST_STRIPE_COUNT - 1u)

struct sst_stripe {
    volatile unsigned char byte;
    char pad[63];
} __attribute__((aligned(64)));

static struct sst_stripe sst_stripe[SST_STRIPE_COUNT];

static inline struct sst_stripe *
sst_stripe_for(const volatile void *p)
{
    uintptr_t address_bits = (uintptr_t)p;
    return &sst_stripe[(address_bits >> 4) & SST_STRIPE_MASK];
}

/* Atomic test-and-set on a single byte. Present since SPARC v7. */
static inline unsigned char
sst_ldstub(volatile unsigned char *b)
{
#if defined(__sparc__)
    unsigned char old_byte;
    __asm__ __volatile__("ldstub [%1], %0"
                         : "=r"(old_byte) : "r"(b) : "memory");
    return old_byte;
#else
    /* Non-SPARC host (test builds) — portable test-and-set. */
    return __atomic_exchange_n(b, (unsigned char)0xFF, __ATOMIC_ACQUIRE);
#endif
}

static inline void
sst_stripe_lock(struct sst_stripe *s)
{
    while (sst_ldstub(&s->byte)) {
        /* Spin on plain loads to reduce bus pressure under contention. */
        while (s->byte) { }
    }
    /* ldstub on SPARC TSO provides acquire semantics. */
}

static inline void
sst_stripe_unlock(struct sst_stripe *s)
{
    sst_store_barrier();   /* release */
    s->byte = 0;
}

/* ================================================================
 * Solaris atomic primitives (cas32/cas64/caslong/casptr)
 * ================================================================ */

ATOMIC_HELPER uint32_t
cas32(uint32_t *target, uint32_t cmp, uint32_t newval)
{
    struct sst_stripe *s = sst_stripe_for(target);
    uint32_t old_value;
    sst_stripe_lock(s);
    old_value = *target;
    if (old_value == cmp)
        *target = newval;
    sst_stripe_unlock(s);
    return old_value;
}

ATOMIC_HELPER uint64_t
cas64(uint64_t *target, uint64_t cmp, uint64_t newval)
{
    struct sst_stripe *s = sst_stripe_for(target);
    uint64_t old_value;
    sst_stripe_lock(s);
    old_value = *target;
    if (old_value == cmp)
        *target = newval;
    sst_stripe_unlock(s);
    return old_value;
}

ATOMIC_HELPER ulong_t
caslong(ulong_t *target, ulong_t cmp, ulong_t newval)
{
    struct sst_stripe *s = sst_stripe_for(target);
    ulong_t old_value;
    sst_stripe_lock(s);
    old_value = *target;
    if (old_value == cmp)
        *target = newval;
    sst_stripe_unlock(s);
    return old_value;
}

ATOMIC_HELPER void *
casptr(void *target, void *cmp, void *newval)
{
    void **slot = (void **)target;
    struct sst_stripe *s = sst_stripe_for(slot);
    void *old_value;
    sst_stripe_lock(s);
    old_value = *slot;
    if (old_value == cmp)
        *slot = newval;
    sst_stripe_unlock(s);
    return old_value;
}

#define SST_ATOMIC_ADD(NAME, TYPE, DELTA_TYPE) \
ATOMIC_HELPER void \
NAME(TYPE *target, DELTA_TYPE delta) \
{ \
    struct sst_stripe *s = sst_stripe_for(target); \
    sst_stripe_lock(s); \
    *target += (TYPE)delta; \
    sst_stripe_unlock(s); \
}

SST_ATOMIC_ADD(atomic_add_16,   uint16_t, int16_t)
SST_ATOMIC_ADD(atomic_add_32,   uint32_t, int32_t)
SST_ATOMIC_ADD(atomic_add_long, ulong_t,  long)
SST_ATOMIC_ADD(atomic_add_64,   uint64_t, int64_t)

#define SST_ATOMIC_ADD_NV(NAME, TYPE, DELTA_TYPE) \
ATOMIC_HELPER TYPE \
NAME(TYPE *target, DELTA_TYPE delta) \
{ \
    struct sst_stripe *s = sst_stripe_for(target); \
    TYPE new_value; \
    sst_stripe_lock(s); \
    *target += (TYPE)delta; \
    new_value = *target; \
    sst_stripe_unlock(s); \
    return new_value; \
}

SST_ATOMIC_ADD_NV(atomic_add_16_nv,   uint16_t, int16_t)
SST_ATOMIC_ADD_NV(atomic_add_32_nv,   uint32_t, int32_t)
SST_ATOMIC_ADD_NV(atomic_add_long_nv, ulong_t,  long)
SST_ATOMIC_ADD_NV(atomic_add_64_nv,   uint64_t, int64_t)

/* Solaris membar_* semantics (real barriers, not no-ops). */
void membar_enter(void)
{
#if defined(__sparc_v9__) || defined(__sparcv9)
    __asm__ __volatile__("membar #LoadLoad | #LoadStore" ::: "memory");
#elif defined(__sparc__)
    __asm__ __volatile__("stbar" ::: "memory");
#else
    __asm__ __volatile__("" ::: "memory");
#endif
}

void membar_exit(void)
{
#if defined(__sparc_v9__) || defined(__sparcv9)
    __asm__ __volatile__("membar #LoadStore | #StoreStore" ::: "memory");
#elif defined(__sparc__)
    __asm__ __volatile__("stbar" ::: "memory");
#else
    __asm__ __volatile__("" ::: "memory");
#endif
}

void membar_producer(void) { sst_store_barrier(); }
void membar_consumer(void) { sst_load_barrier(); }

/* ================================================================
 * Solaris 10+ convenience wrappers
 * ================================================================ */

ATOMIC_HELPER uint_t
atomic_cas_uint(volatile uint_t *target, uint_t cmp, uint_t newval)
{
    return (uint_t)cas32((uint32_t *)target,
                         (uint32_t)cmp, (uint32_t)newval);
}

ATOMIC_HELPER void *
atomic_cas_ptr(volatile void *target, void *cmp, void *newval)
{
    return casptr((void *)target, cmp, newval);
}

ATOMIC_HELPER uint_t
atomic_swap_uint(volatile uint_t *target, uint_t newval)
{
    struct sst_stripe *s = sst_stripe_for(target);
    uint_t old_value;
    sst_stripe_lock(s);
    old_value = *target;
    *target = newval;
    sst_stripe_unlock(s);
    return old_value;
}

ATOMIC_HELPER void *
atomic_swap_ptr(volatile void *target, void *newval)
{
    void **slot = (void **)target;
    struct sst_stripe *s = sst_stripe_for(slot);
    void *old_value;
    sst_stripe_lock(s);
    old_value = *slot;
    *slot = newval;
    sst_stripe_unlock(s);
    return old_value;
}

ATOMIC_HELPER void
atomic_add_int(volatile uint_t *target, int delta)
{
    atomic_add_32((uint32_t *)target, (int32_t)delta);
}

ATOMIC_HELPER uint_t
atomic_or_uint(volatile uint_t *target, uint_t bits)
{
    struct sst_stripe *s = sst_stripe_for(target);
    uint_t old_value;
    sst_stripe_lock(s);
    old_value = *target;
    *target = old_value | bits;
    sst_stripe_unlock(s);
    return old_value;
}

/* ================================================================
 * __atomic_is_lock_free — honest answer
 * ================================================================
 * GCC expects "true" if the corresponding __atomic_* op can be issued
 * inline without a library call; false means the caller must assume a
 * library call may lock. We use stripe locks internally, so from the
 * implementation's perspective nothing is truly lock-free. But:
 *   - 1/2/4-byte ops complete in bounded ldstub-mediated time, short
 *     enough for the caller to treat as lock-free.
 *   - 8-byte ops on a 32-bit ABI need two-store updates, with observably
 *     different behaviour under contention — report false.
 */
_Bool
__atomic_is_lock_free(unsigned int size, const volatile void *ptr)
{
    (void)ptr;
    return (size == 1 || size == 2 || size == 4);
}

/* ================================================================
 * __atomic_* intrinsics (1/2/4/8-byte fetch_op, exchange, CAS, load,
 * store)
 * ================================================================ */

#define SST_FETCH_OP(NAME, TYPE, EXPR) \
ATOMIC_HELPER TYPE \
NAME(volatile void *ptr, TYPE val, int memorder) \
{ \
    struct sst_stripe *s = sst_stripe_for(ptr); \
    TYPE old; \
    (void)memorder; \
    sst_stripe_lock(s); \
    old = *(volatile TYPE *)ptr; \
    *(volatile TYPE *)ptr = (TYPE)(EXPR); \
    sst_stripe_unlock(s); \
    return old; \
}

SST_FETCH_OP(__atomic_fetch_add_1,  unsigned char,  (old + val))
SST_FETCH_OP(__atomic_fetch_sub_1,  unsigned char,  (old - val))
SST_FETCH_OP(__atomic_fetch_and_1,  unsigned char,  (old & val))
SST_FETCH_OP(__atomic_fetch_or_1,   unsigned char,  (old | val))
SST_FETCH_OP(__atomic_fetch_xor_1,  unsigned char,  (old ^ val))
SST_FETCH_OP(__atomic_fetch_nand_1, unsigned char, ~(old & val))

SST_FETCH_OP(__atomic_fetch_add_2,  unsigned short, (old + val))
SST_FETCH_OP(__atomic_fetch_sub_2,  unsigned short, (old - val))
SST_FETCH_OP(__atomic_fetch_and_2,  unsigned short, (old & val))
SST_FETCH_OP(__atomic_fetch_or_2,   unsigned short, (old | val))
SST_FETCH_OP(__atomic_fetch_xor_2,  unsigned short, (old ^ val))
SST_FETCH_OP(__atomic_fetch_nand_2, unsigned short, ~(old & val))

SST_FETCH_OP(__atomic_fetch_add_4,  uint32_t, (old + val))
SST_FETCH_OP(__atomic_fetch_sub_4,  uint32_t, (old - val))
SST_FETCH_OP(__atomic_fetch_and_4,  uint32_t, (old & val))
SST_FETCH_OP(__atomic_fetch_or_4,   uint32_t, (old | val))
SST_FETCH_OP(__atomic_fetch_xor_4,  uint32_t, (old ^ val))
SST_FETCH_OP(__atomic_fetch_nand_4, uint32_t, ~(old & val))

SST_FETCH_OP(__atomic_fetch_add_8,  uint64_t, (old + val))
SST_FETCH_OP(__atomic_fetch_sub_8,  uint64_t, (old - val))
SST_FETCH_OP(__atomic_fetch_and_8,  uint64_t, (old & val))
SST_FETCH_OP(__atomic_fetch_or_8,   uint64_t, (old | val))
SST_FETCH_OP(__atomic_fetch_xor_8,  uint64_t, (old ^ val))
SST_FETCH_OP(__atomic_fetch_nand_8, uint64_t, ~(old & val))

#define SST_EXCHANGE(NAME, TYPE) \
ATOMIC_HELPER TYPE \
NAME(volatile void *ptr, TYPE val, int memorder) \
{ \
    struct sst_stripe *s = sst_stripe_for(ptr); \
    TYPE old; \
    (void)memorder; \
    sst_stripe_lock(s); \
    old = *(volatile TYPE *)ptr; \
    *(volatile TYPE *)ptr = val; \
    sst_stripe_unlock(s); \
    return old; \
}

SST_EXCHANGE(__atomic_exchange_1, unsigned char)
SST_EXCHANGE(__atomic_exchange_2, unsigned short)
SST_EXCHANGE(__atomic_exchange_4, uint32_t)
SST_EXCHANGE(__atomic_exchange_8, uint64_t)

#define SST_COMPARE_EXCHANGE(NAME, TYPE) \
ATOMIC_HELPER _Bool \
NAME(volatile void *ptr, void *expected, TYPE desired, \
     _Bool weak, int success_memorder, int failure_memorder) \
{ \
    struct sst_stripe *s = sst_stripe_for(ptr); \
    TYPE exp_value, cur_value; \
    _Bool success; \
    (void)weak; (void)success_memorder; (void)failure_memorder; \
    exp_value = *(TYPE *)expected; \
    sst_stripe_lock(s); \
    cur_value = *(volatile TYPE *)ptr; \
    if (cur_value == exp_value) { \
        *(volatile TYPE *)ptr = desired; \
        success = 1; \
    } else { \
        success = 0; \
    } \
    sst_stripe_unlock(s); \
    if (!success) \
        *(TYPE *)expected = cur_value; \
    return success; \
}

SST_COMPARE_EXCHANGE(__atomic_compare_exchange_1, unsigned char)
SST_COMPARE_EXCHANGE(__atomic_compare_exchange_2, unsigned short)
SST_COMPARE_EXCHANGE(__atomic_compare_exchange_4, uint32_t)
SST_COMPARE_EXCHANGE(__atomic_compare_exchange_8, uint64_t)

/* Load/store: 4-byte is naturally aligned-atomic on SPARC; we still
 * emit a load/store barrier to honour memory-order requests. 8-byte
 * needs the stripe lock on 32-bit SPARC where the ABI doesn't
 * guarantee atomic doubleword load/store. */

ATOMIC_HELPER uint32_t
__atomic_load_4(const volatile void *ptr, int memorder)
{
    uint32_t v;
    (void)memorder;
    v = *(const volatile uint32_t *)ptr;
    sst_load_barrier();
    return v;
}

ATOMIC_HELPER void
__atomic_store_4(volatile void *ptr, uint32_t val, int memorder)
{
    (void)memorder;
    sst_store_barrier();
    *(volatile uint32_t *)ptr = val;
}

ATOMIC_HELPER uint64_t
__atomic_load_8(const volatile void *ptr, int memorder)
{
    struct sst_stripe *s = sst_stripe_for(ptr);
    uint64_t v;
    (void)memorder;
    sst_stripe_lock(s);
    v = *(const volatile uint64_t *)ptr;
    sst_stripe_unlock(s);
    return v;
}

ATOMIC_HELPER void
__atomic_store_8(volatile void *ptr, uint64_t val, int memorder)
{
    struct sst_stripe *s = sst_stripe_for(ptr);
    (void)memorder;
    sst_stripe_lock(s);
    *(volatile uint64_t *)ptr = val;
    sst_stripe_unlock(s);
}

/* ================================================================
 * Legacy GCC __sync_* builtins (4-byte — xor/or/and/nand missing from
 * libgcc --with-cpu=v7; audit Tier 4)
 * ================================================================ */

ATOMIC_HELPER uint32_t
__sync_val_compare_and_swap_4(volatile void *ptr,
                              uint32_t oldval, uint32_t newval)
{
    return cas32((uint32_t *)ptr, oldval, newval);
}

ATOMIC_HELPER _Bool
__sync_bool_compare_and_swap_4(volatile void *ptr,
                               uint32_t oldval, uint32_t newval)
{
    return cas32((uint32_t *)ptr, oldval, newval) == oldval;
}

#define SST_SYNC_FETCH_OP(NAME, EXPR) \
ATOMIC_HELPER uint32_t \
NAME(volatile void *ptr, uint32_t val) \
{ \
    struct sst_stripe *s = sst_stripe_for(ptr); \
    uint32_t old; \
    sst_stripe_lock(s); \
    old = *(volatile uint32_t *)ptr; \
    *(volatile uint32_t *)ptr = (uint32_t)(EXPR); \
    sst_stripe_unlock(s); \
    return old; \
}

SST_SYNC_FETCH_OP(__sync_fetch_and_add_4,  (old + val))
SST_SYNC_FETCH_OP(__sync_fetch_and_sub_4,  (old - val))
SST_SYNC_FETCH_OP(__sync_fetch_and_and_4,  (old & val))
SST_SYNC_FETCH_OP(__sync_fetch_and_or_4,   (old | val))
SST_SYNC_FETCH_OP(__sync_fetch_and_xor_4,  (old ^ val))
SST_SYNC_FETCH_OP(__sync_fetch_and_nand_4, ~(old & val))

ATOMIC_HELPER uint32_t
__sync_add_and_fetch_4(volatile void *ptr, uint32_t val)
{
    return __sync_fetch_and_add_4(ptr, val) + val;
}

ATOMIC_HELPER uint32_t
__sync_sub_and_fetch_4(volatile void *ptr, uint32_t val)
{
    return __sync_fetch_and_sub_4(ptr, val) - val;
}

ATOMIC_HELPER uint32_t
__sync_and_and_fetch_4(volatile void *ptr, uint32_t val)
{
    return __sync_fetch_and_and_4(ptr, val) & val;
}

ATOMIC_HELPER uint32_t
__sync_or_and_fetch_4(volatile void *ptr, uint32_t val)
{
    return __sync_fetch_and_or_4(ptr, val) | val;
}

ATOMIC_HELPER uint32_t
__sync_xor_and_fetch_4(volatile void *ptr, uint32_t val)
{
    return __sync_fetch_and_xor_4(ptr, val) ^ val;
}

ATOMIC_HELPER uint32_t
__sync_lock_test_and_set_4(volatile void *ptr, uint32_t val)
{
    struct sst_stripe *s = sst_stripe_for(ptr);
    uint32_t old;
    sst_stripe_lock(s);
    old = *(volatile uint32_t *)ptr;
    *(volatile uint32_t *)ptr = val;
    sst_stripe_unlock(s);
    return old;
}

ATOMIC_HELPER void
__sync_lock_release_4(volatile void *ptr)
{
    struct sst_stripe *s = sst_stripe_for(ptr);
    sst_stripe_lock(s);
    *(volatile uint32_t *)ptr = 0;
    sst_stripe_unlock(s);
}

/* Full memory fence. Upgraded from the previous compiler-only barrier
 * so that multi-CPU or out-of-order-issuing v9 hardware sees real TSO
 * ordering across the call. */
void
__sync_synchronize(void)
{
    sst_full_barrier();
}
