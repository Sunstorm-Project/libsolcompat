/*
 * cxa_atexit.c — __cxa_atexit / __cxa_finalize for Solaris 7
 *
 * Solaris 7 libc provides atexit() but not the C++ ABI functions
 * __cxa_atexit and __cxa_finalize. These are needed by GCC's C++
 * runtime for proper per-DSO destructor ordering of local statics.
 *
 * __cxa_atexit(func, arg, dso_handle):
 *   Register a destructor 'func(arg)' associated with DSO 'dso_handle'.
 *   Called by the compiler for static locals with non-trivial destructors.
 *
 * __cxa_finalize(dso_handle):
 *   Run all destructors registered for 'dso_handle' (in reverse order).
 *   If dso_handle is NULL, run ALL registered destructors.
 *   Called by dlclose() or at program exit.
 *
 * This implementation uses a simple linked list protected by a mutex.
 * Sufficient for the single-process, mostly-static SST environment.
 */

#include <stdlib.h>
#include <pthread.h>

/* Maximum number of atexit handlers. The C standard requires at least 32;
   glibc supports 65536. 1024 is generous for our use case. */
#define CXA_ATEXIT_MAX 1024

struct cxa_atexit_entry {
    void (*destructor_func)(void *);
    void *object_arg;
    void *dso_handle;
    int in_use;
};

static struct cxa_atexit_entry cxa_atexit_table[CXA_ATEXIT_MAX];
static int cxa_atexit_count = 0;
static pthread_mutex_t cxa_atexit_lock = PTHREAD_MUTEX_INITIALIZER;
static int cxa_finalize_registered = 0;

/* Forward declaration */
void __cxa_finalize(void *dso_handle);

/* Called at program exit to run all remaining destructors */
static void
cxa_atexit_cleanup(void)
{
    __cxa_finalize((void *)0);
}

/*
 * __cxa_atexit — register a destructor for a static local object
 *
 * Returns 0 on success, -1 on failure.
 */
int
__cxa_atexit(void (*destructor_func)(void *), void *object_arg, void *dso_handle)
{
    int result = -1;

    pthread_mutex_lock(&cxa_atexit_lock);

    /* Register our cleanup with plain atexit() on first call */
    if (!cxa_finalize_registered) {
        atexit(cxa_atexit_cleanup);
        cxa_finalize_registered = 1;
    }

    if (cxa_atexit_count < CXA_ATEXIT_MAX) {
        struct cxa_atexit_entry *entry = &cxa_atexit_table[cxa_atexit_count];
        entry->destructor_func = destructor_func;
        entry->object_arg = object_arg;
        entry->dso_handle = dso_handle;
        entry->in_use = 1;
        cxa_atexit_count++;
        result = 0;
    }

    pthread_mutex_unlock(&cxa_atexit_lock);
    return result;
}

/*
 * __cxa_finalize — run destructors for a given DSO, or all if NULL
 *
 * Runs in reverse registration order per the C++ ABI spec.
 */
void
__cxa_finalize(void *dso_handle)
{
    int entry_index;

    pthread_mutex_lock(&cxa_atexit_lock);

    /* Walk backwards for LIFO order */
    for (entry_index = cxa_atexit_count - 1; entry_index >= 0; entry_index--) {
        struct cxa_atexit_entry *entry = &cxa_atexit_table[entry_index];

        if (!entry->in_use)
            continue;

        /* NULL dso_handle means run everything; otherwise match DSO */
        if (dso_handle != (void *)0 && entry->dso_handle != dso_handle)
            continue;

        /* Mark as consumed before calling (prevents re-entry issues) */
        entry->in_use = 0;

        /* Release lock during callback (destructor may call __cxa_atexit) */
        pthread_mutex_unlock(&cxa_atexit_lock);
        entry->destructor_func(entry->object_arg);
        pthread_mutex_lock(&cxa_atexit_lock);
    }

    pthread_mutex_unlock(&cxa_atexit_lock);
}
