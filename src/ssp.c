/*
 * ssp.c — Stack Smashing Protector support for Solaris 7
 *
 * Provides __stack_chk_guard (the canary value) and __stack_chk_fail
 * (the handler called when stack corruption is detected).
 *
 * GCC's -fstack-protector-* emits code that checks __stack_chk_guard
 * at function entry/exit. On corruption, __stack_chk_fail is called.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */
#include <stdlib.h>
#include <stdarg.h>   /* before stdio.h: defines __gnuc_va_list */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

/* The canary value — initialized at load time via constructor */
void *__stack_chk_guard = 0;

/*
 * Mix multiple weak entropy sources into a single canary.
 *
 * The previous fallback wrote a fixed pattern (0x00, 0x00, '\n', 0xff)
 * whenever /dev/urandom was unavailable. On base Solaris 7 without
 * SUNWski, /dev/urandom is always absent — which meant every process
 * linked against libsolcompat shared the SAME guard value across the
 * entire userland. That reduces SSP to a no-op: an attacker who can
 * read libsolcompat can hardcode the guard bytes and trivially bypass
 * -fstack-protector.
 *
 * This fallback mixes gethrtime (nanosecond wall clock), getpid,
 * time(NULL), and the address of a local variable (ASLR-ish heap
 * layout) into a 4-byte canary. Still NOT cryptographic — the goal
 * is per-process variation, not guessing resistance. A real SPARC
 * Solaris 7 box should run prngd (or ship SUNWski) and get
 * /dev/urandom-backed entropy above; this is the "fell through"
 * safety net.
 */
static void
mix_weak_canary_bytes(unsigned char *out)
{
    long long hrt;
    unsigned long mix;
    unsigned char local;

    hrt = gethrtime();
    mix = (unsigned long)hrt;
    mix ^= ((unsigned long)hrt >> 32);
    mix ^= (unsigned long)getpid() * 2654435761UL;    /* Knuth mul */
    mix ^= (unsigned long)time(NULL) * 16777619UL;    /* FNV prime */
    mix ^= (unsigned long)(uintptr_t)&local;

    out[0] = (unsigned char)(mix >> 0);
    out[1] = (unsigned char)(mix >> 8);
    out[2] = (unsigned char)(mix >> 16);
    out[3] = (unsigned char)(mix >> 24);
    /* Force the low byte to 0 so the canary detects string-overflow
     * NUL-termination attacks (glibc convention); the other 3 bytes
     * carry the per-process variation. */
    out[0] = 0;
}

static void __attribute__((constructor))
__stack_chk_guard_init(void)
{
    int fd;
    /* Try /dev/urandom first (if prngd is running) */
    fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        if (read(fd, &__stack_chk_guard, sizeof(__stack_chk_guard))
            == (ssize_t)sizeof(__stack_chk_guard)) {
            close(fd);
            /* Ensure at least one byte is nonzero to catch string overflows */
            if (__stack_chk_guard != 0)
                return;
        }
        close(fd);
    }
    /* Fallback: per-process weak canary. Better than the fixed
     * 0x00,0x00,'\n',0xff pattern (which was identical across every
     * process and made SSP a no-op). Still not cryptographic. */
    mix_weak_canary_bytes((unsigned char *)&__stack_chk_guard);
}

void __attribute__((noreturn))
__stack_chk_fail(void)
{
    const char msg[] = "*** stack smashing detected ***: terminated\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    abort();
}
