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

/* The canary value — initialized at load time via constructor */
void *__stack_chk_guard = 0;

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
    /* Fallback: use a fixed pattern with a null byte at the bottom
     * to catch string-based overflows (same pattern as glibc fallback) */
    ((unsigned char *)&__stack_chk_guard)[0] = 0;
    ((unsigned char *)&__stack_chk_guard)[1] = 0;
    ((unsigned char *)&__stack_chk_guard)[2] = '\n';
    ((unsigned char *)&__stack_chk_guard)[3] = 0xff;
}

void __attribute__((noreturn))
__stack_chk_fail(void)
{
    const char msg[] = "*** stack smashing detected ***: terminated\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    abort();
}
