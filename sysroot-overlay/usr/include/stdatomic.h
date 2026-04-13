/*
 * stdatomic.h — C11 atomics for Solaris 7
 *
 * GCC 15 handles _Atomic keyword and __atomic_* builtins natively,
 * but does not ship a stdatomic.h for this target. Python, curl,
 * fontconfig, freetype, and openssl include <stdatomic.h> directly.
 * This header wraps GCC's builtins with the C11-standard names.
 * Modeled on glibc's and GCC's own stdatomic.h for other targets.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SOLCOMPAT_STDATOMIC_H
#define _SOLCOMPAT_STDATOMIC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Memory order constants — match __ATOMIC_* values so GCC builtins pass through. */
typedef enum memory_order {
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_consume = __ATOMIC_CONSUME,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order;

/* Atomic-qualified typedefs */
typedef _Atomic _Bool        atomic_bool;
typedef _Atomic char         atomic_char;
typedef _Atomic signed char  atomic_schar;
typedef _Atomic unsigned char atomic_uchar;
typedef _Atomic short        atomic_short;
typedef _Atomic unsigned short atomic_ushort;
typedef _Atomic int          atomic_int;
typedef _Atomic unsigned int atomic_uint;
typedef _Atomic long         atomic_long;
typedef _Atomic unsigned long atomic_ulong;
typedef _Atomic long long    atomic_llong;
typedef _Atomic unsigned long long atomic_ullong;
typedef _Atomic size_t       atomic_size_t;
typedef _Atomic ptrdiff_t    atomic_ptrdiff_t;
typedef _Atomic intptr_t     atomic_intptr_t;
typedef _Atomic uintptr_t    atomic_uintptr_t;
typedef _Atomic int32_t      atomic_int32_t;
typedef _Atomic uint32_t     atomic_uint32_t;
typedef _Atomic int64_t      atomic_int64_t;
typedef _Atomic uint64_t     atomic_uint64_t;

typedef _Atomic struct { unsigned char _v; } atomic_flag;
#define ATOMIC_FLAG_INIT { 0 }

#define ATOMIC_VAR_INIT(v) (v)
#define atomic_init(obj, value) \
    __c11_atomic_init((obj), (value))

#define atomic_thread_fence(order) __atomic_thread_fence((order))
#define atomic_signal_fence(order) __atomic_signal_fence((order))

#define atomic_is_lock_free(obj) \
    __atomic_is_lock_free(sizeof(*(obj)), (obj))

#define atomic_store_explicit(obj, desired, order) \
    __atomic_store_n((obj), (desired), (order))
#define atomic_store(obj, desired) \
    atomic_store_explicit((obj), (desired), memory_order_seq_cst)

#define atomic_load_explicit(obj, order) \
    __atomic_load_n((obj), (order))
#define atomic_load(obj) \
    atomic_load_explicit((obj), memory_order_seq_cst)

#define atomic_exchange_explicit(obj, desired, order) \
    __atomic_exchange_n((obj), (desired), (order))
#define atomic_exchange(obj, desired) \
    atomic_exchange_explicit((obj), (desired), memory_order_seq_cst)

#define atomic_compare_exchange_strong_explicit(obj, expected, desired, success, failure) \
    __atomic_compare_exchange_n((obj), (expected), (desired), 0, (success), (failure))
#define atomic_compare_exchange_strong(obj, expected, desired) \
    atomic_compare_exchange_strong_explicit((obj), (expected), (desired), \
        memory_order_seq_cst, memory_order_seq_cst)

#define atomic_compare_exchange_weak_explicit(obj, expected, desired, success, failure) \
    __atomic_compare_exchange_n((obj), (expected), (desired), 1, (success), (failure))
#define atomic_compare_exchange_weak(obj, expected, desired) \
    atomic_compare_exchange_weak_explicit((obj), (expected), (desired), \
        memory_order_seq_cst, memory_order_seq_cst)

#define atomic_fetch_add_explicit(obj, val, order) __atomic_fetch_add((obj), (val), (order))
#define atomic_fetch_add(obj, val) atomic_fetch_add_explicit((obj), (val), memory_order_seq_cst)
#define atomic_fetch_sub_explicit(obj, val, order) __atomic_fetch_sub((obj), (val), (order))
#define atomic_fetch_sub(obj, val) atomic_fetch_sub_explicit((obj), (val), memory_order_seq_cst)
#define atomic_fetch_or_explicit(obj, val, order)  __atomic_fetch_or((obj), (val), (order))
#define atomic_fetch_or(obj, val)  atomic_fetch_or_explicit((obj), (val), memory_order_seq_cst)
#define atomic_fetch_xor_explicit(obj, val, order) __atomic_fetch_xor((obj), (val), (order))
#define atomic_fetch_xor(obj, val) atomic_fetch_xor_explicit((obj), (val), memory_order_seq_cst)
#define atomic_fetch_and_explicit(obj, val, order) __atomic_fetch_and((obj), (val), (order))
#define atomic_fetch_and(obj, val) atomic_fetch_and_explicit((obj), (val), memory_order_seq_cst)

#define atomic_flag_test_and_set_explicit(obj, order) \
    __atomic_test_and_set(&(obj)->_v, (order))
#define atomic_flag_test_and_set(obj) \
    atomic_flag_test_and_set_explicit((obj), memory_order_seq_cst)
#define atomic_flag_clear_explicit(obj, order) \
    __atomic_clear(&(obj)->_v, (order))
#define atomic_flag_clear(obj) \
    atomic_flag_clear_explicit((obj), memory_order_seq_cst)

#endif /* _SOLCOMPAT_STDATOMIC_H */
