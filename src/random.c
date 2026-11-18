/*
 * random.c — Cryptographic and general-purpose randomness for Solaris 7
 *
 * Secure bytes come from a Solaris random device or the PRNGD socket.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

/* Solaris gethrtime for entropy mixing */
extern long long gethrtime(void);

/*
 * EGD (Entropy Gathering Daemon) client.
 *
 * Solaris 7 ships no /dev/urandom and no /dev/random without SUNWski.
 * The SST toolchain installs `prngd` which exposes an EGD-protocol
 * Unix socket (default /var/run/egd-pool).  Python, OpenSSL, OpenSSH,
 * and anything else that calls getentropy / getrandom otherwise fails
 * loudly.  Read from the EGD socket so those callers get real entropy.
 *
 * EGD protocol (per draft-eastlake-randomness-04 / PRNGD docs):
 *   command 0x02 ("blocking read, arbitrary"):
 *     client -> server:  byte 0x02, byte count B (1..255)
 *     server -> client:  B bytes of random data (no length prefix)
 *
 *   NOTE: command 0x01 (non-blocking read) DOES prefix the reply with
 *   a length byte S which may be < B.  We use 0x02 so we always get
 *   exactly what we asked for, loop externally for counts > 255.
 *
 * Socket path override: SOLCOMPAT_EGD_SOCKET env var, else default
 * /var/run/egd-pool.
 */
#define EGD_DEFAULT_SOCKET "/var/run/egd-pool"
#define EGD_MAX_REQUEST    255

static int
egd_connect(void)
{
    const char *socket_path = NULL;
    if (getuid() == geteuid() && getgid() == getegid())
        socket_path = getenv("SOLCOMPAT_EGD_SOCKET");
    if (socket_path == NULL || *socket_path == '\0')
        socket_path = EGD_DEFAULT_SOCKET;

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    /* Reject overflow — path too long means nonsense configuration. */
    if (strlen(socket_path) >= sizeof(addr.sun_path)) {
        close(sock);
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(addr.sun_path, socket_path);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        int saved_errno = errno;
        close(sock);
        errno = saved_errno;
        return -1;
    }
    return sock;
}

/*
 * Read exactly 'len' bytes from EGD socket.  Returns 0 on success,
 * -1 on any error (connection refused, short read, server returned
 * fewer bytes than requested).  Caller falls back to other sources
 * on failure.
 */
static int
egd_get_bytes(void *buf, size_t len)
{
    int sock = egd_connect();
    if (sock < 0)
        return -1;

    unsigned char *output = (unsigned char *)buf;
    while (len > 0) {
        unsigned char request[2];
        size_t chunk = len > EGD_MAX_REQUEST ? EGD_MAX_REQUEST : len;
        request[0] = 0x02;                  /* blocking read */
        request[1] = (unsigned char)chunk;

        /* Send request — EGD expects both bytes in one write. */
        if (write(sock, request, 2) != 2) {
            close(sock);
            return -1;
        }

        /* Command 0x02 reply is exactly `chunk` bytes of random data,
         * no length prefix.  Read until we have them all; loop on EINTR
         * and handle short reads explicitly. */
        size_t got = 0;
        while (got < chunk) {
            ssize_t r = read(sock, output + got, chunk - got);
            if (r < 0) {
                if (errno == EINTR) continue;
                close(sock);
                return -1;
            }
            if (r == 0) {
                close(sock);
                return -1;
            }
            got += (size_t)r;
        }

        output += chunk;
        len -= chunk;
    }

    close(sock);
    return 0;
}

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
 * Try to get random bytes from the kernel or EGD.
 *
 * Preference order:
 *   1. /dev/urandom  (Solaris 9+ or SUNWski-patched 7/8)
 *   2. /dev/random   (SUNWski on 7/8)
 *   3. EGD socket    (prngd pool — SST ships this)
 *
 * Open a fresh source per request so failures are retryable and forked
 * processes and threads never share generator state or cached descriptors.
 */
static int
get_random_bytes(void *buf, size_t len)
{
    static const char *const devices[] = { "/dev/urandom", "/dev/random" };
    size_t i;

    if (len == 0)
        return 0;
    for (i = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
        int fd = open(devices[i], O_RDONLY);
        if (fd >= 0) {
            int result = read_all(fd, buf, len);
            close(fd);
            if (result == 0)
                return 0;
        }
    }
    return egd_get_bytes(buf, len);
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

    /*
     * No /dev/random or /dev/urandom available.
     *
     * Silently falling back to the LCG was a cryptographic footgun:
     * OpenSSL / OpenSSH / Python's `secrets` / any caller of
     * getentropy() may use the result as seed material. If this function silently
     * returns LCG output, session keys / nonces / CSRF tokens become
     * predictable with zero diagnostic.  A loud failure is strictly
     * better than a silent weak-entropy substitution.
     *
     * If a caller explicitly wants the predictable fallback (e.g. for
     * testing), they can set SOLCOMPAT_ALLOW_WEAK_RANDOM=1.
     *
     * Base Solaris 7 without SUNWski has neither /dev/random nor
     * /dev/urandom, so this path is hit by default on vanilla installs.
     * The right remediation is to ship SUNWski or a userspace RNG;
     * libsolcompat refuses to paper over its absence with an LCG.
     */
    if (getenv("SOLCOMPAT_ALLOW_WEAK_RANDOM") != NULL) {
        fallback_random_bytes(buffer, length);
        return 0;
    }
    errno = EIO;
    return -1;
}

/*
 * getrandom — Linux 3.17 syscall interface. Solaris 7 has no equivalent.
 * We delegate to get_random_bytes() (which tries /dev/urandom, /dev/random,
 * then the EGD pool) and ignore the flags — all three (GRND_NONBLOCK,
 * GRND_RANDOM, GRND_INSECURE) are approximately satisfied because every
 * source we use is non-blocking and kernel-pseudo-random.  Returns the
 * number of bytes written, or -1 / errno on error.
 *
 * gnulib's getrandom.c and many packages (bash, coreutils, Python,
 * git, openssl, openssh, gettext, sudo, wget) include <sys/random.h>
 * unconditionally and call this.  Python in particular calls it during
 * interpreter init for hash randomization; returning a weak LCG there
 * is the same cryptographic footgun described on getentropy above, so
 * we fail loudly rather than silently substitute.
 */
ssize_t
getrandom(void *buf, size_t buflen, unsigned int flags)
{
    (void)flags;
    if (get_random_bytes(buf, buflen) == 0)
        return (ssize_t)buflen;

    if (getenv("SOLCOMPAT_ALLOW_WEAK_RANDOM") != NULL) {
        fallback_random_bytes(buf, buflen);
        return (ssize_t)buflen;
    }
    errno = EIO;
    return -1;
}

/* These APIs have no error return. Never expose partially filled output
 * or use the optional weak test generator when the entropy source fails. */
void
arc4random_buf(void *buf, size_t nbytes)
{
    if (get_random_bytes(buf, nbytes) < 0) {
        explicit_bzero(buf, nbytes);
        abort();
    }
}

uint32_t
arc4random(void)
{
    uint32_t value;
    arc4random_buf(&value, sizeof(value));
    return value;
}

uint32_t
arc4random_uniform(uint32_t upper_bound)
{
    uint32_t min, r;

    if (upper_bound < 2)
        return 0;

    /* Avoid modulo bias */
    min = (uint32_t)(0U - upper_bound) % upper_bound;
    for (;;) {
        r = arc4random();
        if (r >= min)
            return r % upper_bound;
    }
}
