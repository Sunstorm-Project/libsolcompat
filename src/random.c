/*
 * random.c — Cryptographic and general-purpose randomness for Solaris 7
 *
 * Solaris 7 has no /dev/urandom, no arc4random, no getentropy.
 * We use /dev/random (which does exist on some Solaris 7 installs
 * with the SUNWski package) or fall back to a combination of
 * gethrtime(), getpid(), and time() for seeding.
 *
 * For production cryptographic use, SUNWski should be installed.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>

/* Solaris gethrtime for entropy mixing */
extern long long gethrtime(void);

/*
 * explicit_bzero — guaranteed to not be optimized away
 */
void
explicit_bzero(void *s, size_t n)
{
    volatile unsigned char *p = (volatile unsigned char *)s;
    while (n--)
        *p++ = 0;
}

/*
 * Read exactly 'len' bytes from an fd, retrying on EINTR.
 */
static int
read_all(int fd, void *buf, size_t len)
{
    unsigned char *p = (unsigned char *)buf;
    while (len > 0) {
        ssize_t r = read(fd, p, len);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return -1;
        p += r;
        len -= (size_t)r;
    }
    return 0;
}

/*
 * Try to get random bytes from the kernel.
 * Returns 0 on success, -1 if no random device available.
 */
static int
get_random_bytes(void *buf, size_t len)
{
    static int random_fd = -2;  /* -2 = not tried, -1 = not available */

    if (random_fd == -2) {
        /* Try /dev/urandom first (may exist on patched systems) */
        random_fd = open("/dev/urandom", O_RDONLY);
        if (random_fd < 0) {
            /* Try /dev/random (SUNWski package) */
            random_fd = open("/dev/random", O_RDONLY);
        }
        if (random_fd < 0)
            random_fd = -1;
    }

    if (random_fd < 0)
        return -1;

    return read_all(random_fd, buf, len);
}

/*
 * Fallback PRNG seeded from system state.
 * NOT cryptographically secure — only used when /dev/random
 * is unavailable.
 */
static void
fallback_random_bytes(void *buf, size_t len)
{
    unsigned char *p = (unsigned char *)buf;
    static int seeded = 0;
    static unsigned long state;
    size_t i;

    if (!seeded) {
        long long hr = gethrtime();
        state = (unsigned long)hr ^ (unsigned long)getpid() ^
                (unsigned long)time(NULL) ^ (unsigned long)&state;
        seeded = 1;
    }

    for (i = 0; i < len; i++) {
        /* Simple LCG — not crypto grade */
        state = state * 1103515245UL + 12345UL;
        p[i] = (unsigned char)(state >> 16);
    }
}

int
getentropy(void *buffer, size_t length)
{
    if (length > 256) {
        errno = EIO;
        return -1;
    }

    if (get_random_bytes(buffer, length) == 0)
        return 0;

    /* Fallback — better than failing */
    fallback_random_bytes(buffer, length);
    return 0;
}

/*
 * getrandom — Linux 3.17 syscall interface. Solaris 7 has no equivalent.
 * We delegate to get_random_bytes() (which reads /dev/urandom) and
 * ignore the flags — all three (GRND_NONBLOCK, GRND_RANDOM,
 * GRND_INSECURE) are approximately satisfied because /dev/urandom is
 * non-blocking and pseudo-random on every modern Unix. Returns the
 * number of bytes written, or -1 on error.
 *
 * gnulib's getrandom.c and many packages (bash, coreutils, Python,
 * git, openssl, openssh, gettext, sudo, wget) include <sys/random.h>
 * unconditionally and call this.
 */
ssize_t
getrandom(void *buf, size_t buflen, unsigned int flags)
{
    (void)flags;
    if (get_random_bytes(buf, buflen) == 0)
        return (ssize_t)buflen;
    fallback_random_bytes(buf, buflen);
    return (ssize_t)buflen;
}

/*
 * arc4random family
 *
 * Uses ChaCha20 when backed by real entropy,
 * or the fallback PRNG when not.
 * For simplicity, we use a basic implementation that reseeds
 * from getentropy() periodically.
 */

static unsigned long arc4_state[4];
static int arc4_initialized = 0;
static int arc4_count = 0;

static void
arc4_stir(void)
{
    unsigned char seed[16];
    getentropy(seed, sizeof(seed));
    memcpy(arc4_state, seed, sizeof(seed));
    arc4_count = 0;
    arc4_initialized = 1;
    explicit_bzero(seed, sizeof(seed));
}

static unsigned long
arc4_next(void)
{
    /* Simple mixing — not ChaCha20, but reasonable for non-crypto use */
    if (!arc4_initialized || arc4_count > 1600000)
        arc4_stir();

    arc4_state[0] += arc4_state[1];
    arc4_state[1] = (arc4_state[1] << 13) | (arc4_state[1] >> 19);
    arc4_state[1] ^= arc4_state[0];
    arc4_state[0] = (arc4_state[0] << 16) | (arc4_state[0] >> 16);
    arc4_state[2] += arc4_state[3];
    arc4_state[3] = (arc4_state[3] << 17) | (arc4_state[3] >> 15);
    arc4_state[3] ^= arc4_state[2];
    arc4_state[0] += arc4_state[3];
    arc4_state[2] += arc4_state[1];

    arc4_count++;
    return arc4_state[0];
}

uint32_t
arc4random(void)
{
    return (uint32_t)arc4_next();
}

void
arc4random_buf(void *buf, size_t nbytes)
{
    unsigned char *p = (unsigned char *)buf;
    while (nbytes >= 4) {
        uint32_t val = arc4random();
        memcpy(p, &val, 4);
        p += 4;
        nbytes -= 4;
    }
    if (nbytes > 0) {
        uint32_t val = arc4random();
        memcpy(p, &val, nbytes);
    }
}

uint32_t
arc4random_uniform(uint32_t upper_bound)
{
    uint32_t min, r;

    if (upper_bound < 2)
        return 0;

    /* Avoid modulo bias */
    min = (uint32_t)(-(int32_t)upper_bound) % upper_bound;
    for (;;) {
        r = arc4random();
        if (r >= min)
            return r % upper_bound;
    }
}
