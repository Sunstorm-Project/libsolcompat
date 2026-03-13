/*
 * test_all.c — Test suite for libsolcompat
 *
 * Compile: cc -I../include -I../include/override -o test_all test_all.c -L.. -lsolcompat -lrt -lsocket -lnsl -ldl -lm
 * Run:     ./test_all
 *
 * Each subsystem has a test_*() function.  On failure, prints the
 * test name and exits with code 1.  On success, prints "PASS".
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

/* Include the master header — this pulls in everything */
#include <solcompat/solcompat.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("  %-40s ", #name); \
        fflush(stdout); \
    } while (0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while (0)

#define FAIL(msg) \
    do { \
        printf("FAIL: %s\n", msg); \
    } while (0)

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { FAIL(msg); return; } \
    } while (0)

/* ===== snprintf tests ===== */
static void test_snprintf_basic(void)
{
    char buf[32];
    int n;

    TEST(snprintf_basic);
    n = snprintf(buf, sizeof(buf), "hello %s", "world");
    ASSERT(n == 11, "expected 11");
    ASSERT(strcmp(buf, "hello world") == 0, "wrong output");
    PASS();
}

static void test_snprintf_truncation(void)
{
    char buf[8];
    int n;

    TEST(snprintf_truncation);
    n = snprintf(buf, sizeof(buf), "hello world, this is long");
    ASSERT(n == 25, "should return 25 (full length)");
    ASSERT(buf[7] == '\0', "should be NUL terminated");
    ASSERT(strncmp(buf, "hello w", 7) == 0, "wrong truncated output");
    PASS();
}

static void test_snprintf_null_measure(void)
{
    int n;

    TEST(snprintf_null_measure);
    n = snprintf(NULL, 0, "test %d %s", 42, "string");
    ASSERT(n > 0, "should return positive count");
    ASSERT(n == 14, "expected 14");
    PASS();
}

/* ===== string tests ===== */
static void test_strndup(void)
{
    char *s;

    TEST(strndup);
    s = strndup("hello world", 5);
    ASSERT(s != NULL, "returned NULL");
    ASSERT(strcmp(s, "hello") == 0, "wrong result");
    free(s);
    PASS();
}

static void test_strnlen(void)
{
    TEST(strnlen);
    ASSERT(strnlen("hello", 10) == 5, "wrong for short string");
    ASSERT(strnlen("hello", 3) == 3, "wrong for limited maxlen");
    PASS();
}

static void test_strlcpy(void)
{
    char buf[8];

    TEST(strlcpy);
    ASSERT(strlcpy(buf, "hello", sizeof(buf)) == 5, "wrong return");
    ASSERT(strcmp(buf, "hello") == 0, "wrong copy");
    ASSERT(strlcpy(buf, "very long string", sizeof(buf)) == 16, "should return src len");
    ASSERT(buf[7] == '\0', "not NUL terminated");
    PASS();
}

static void test_strlcat(void)
{
    char buf[16];

    TEST(strlcat);
    strlcpy(buf, "hello", sizeof(buf));
    ASSERT(strlcat(buf, " world", sizeof(buf)) == 11, "wrong return");
    ASSERT(strcmp(buf, "hello world") == 0, "wrong concat");
    PASS();
}

static void test_strcasestr(void)
{
    TEST(strcasestr);
    ASSERT(strcasestr("Hello World", "WORLD") != NULL, "should find");
    ASSERT(strcasestr("Hello World", "xyz") == NULL, "should not find");
    PASS();
}

static void test_memmem(void)
{
    TEST(memmem);
    ASSERT(memmem("hello world", 11, "world", 5) != NULL, "should find");
    ASSERT(memmem("hello world", 11, "xyz", 3) == NULL, "should not find");
    PASS();
}

static void test_strsep(void)
{
    char buf[] = "one:two:three";
    char *p = buf;
    char *tok;

    TEST(strsep);
    tok = strsep(&p, ":");
    ASSERT(strcmp(tok, "one") == 0, "first token wrong");
    tok = strsep(&p, ":");
    ASSERT(strcmp(tok, "two") == 0, "second token wrong");
    tok = strsep(&p, ":");
    ASSERT(strcmp(tok, "three") == 0, "third token wrong");
    ASSERT(p == NULL, "should be NULL after last token");
    PASS();
}

static void test_memrchr(void)
{
    TEST(memrchr);
    ASSERT(memrchr("hello", 'l', 5) == (void *)("hello" + 3), "should find last l");
    ASSERT(memrchr("hello", 'z', 5) == NULL, "should not find z");
    PASS();
}

/* ===== stdio tests ===== */
static void test_vasprintf_wrapper(void)
{
    char *s;
    int n;

    TEST(asprintf);
    n = asprintf(&s, "number %d and %s", 42, "str");
    ASSERT(n > 0, "should return positive");
    ASSERT(s != NULL, "should allocate");
    ASSERT(strcmp(s, "number 42 and str") == 0, "wrong output");
    free(s);
    PASS();
}

static void test_getline(void)
{
    FILE *f;
    char *line = NULL;
    size_t n = 0;
    ssize_t len;

    TEST(getline);
    f = tmpfile();
    ASSERT(f != NULL, "tmpfile failed");

    fputs("line one\nline two\n", f);
    rewind(f);

    len = getline(&line, &n, f);
    ASSERT(len > 0, "getline returned <= 0");
    ASSERT(strcmp(line, "line one\n") == 0, "wrong first line");

    len = getline(&line, &n, f);
    ASSERT(len > 0, "getline returned <= 0 for second line");
    ASSERT(strcmp(line, "line two\n") == 0, "wrong second line");

    free(line);
    fclose(f);
    PASS();
}

/* ===== stdlib tests ===== */
static void test_setenv_unsetenv(void)
{
    TEST(setenv_unsetenv);
    ASSERT(setenv("SOLCOMPAT_TEST_VAR", "hello", 1) == 0, "setenv failed");
    ASSERT(strcmp(getenv("SOLCOMPAT_TEST_VAR"), "hello") == 0, "wrong value");
    ASSERT(setenv("SOLCOMPAT_TEST_VAR", "world", 0) == 0, "setenv no-overwrite failed");
    ASSERT(strcmp(getenv("SOLCOMPAT_TEST_VAR"), "hello") == 0, "should not overwrite");
    ASSERT(unsetenv("SOLCOMPAT_TEST_VAR") == 0, "unsetenv failed");
    ASSERT(getenv("SOLCOMPAT_TEST_VAR") == NULL, "should be gone");
    PASS();
}

static void test_mkdtemp(void)
{
    char tmpl[] = "/tmp/solcompat_test_XXXXXX";

    TEST(mkdtemp);
    ASSERT(mkdtemp(tmpl) != NULL, "mkdtemp returned NULL");
    /* Verify directory exists */
    {
        struct stat st;
        ASSERT(stat(tmpl, &st) == 0, "dir doesn't exist");
        ASSERT(S_ISDIR(st.st_mode), "not a directory");
    }
    rmdir(tmpl);
    PASS();
}

/* ===== clock tests ===== */
static void test_clock_monotonic(void)
{
    struct timespec ts;

    TEST(clock_monotonic);
    ASSERT(clock_gettime(CLOCK_MONOTONIC, &ts) == 0, "clock_gettime failed");
    ASSERT(ts.tv_sec > 0 || ts.tv_nsec > 0, "time should be non-zero");
    PASS();
}

static void test_clock_realtime(void)
{
    struct timespec ts;

    TEST(clock_realtime);
    ASSERT(clock_gettime(CLOCK_REALTIME, &ts) == 0, "clock_gettime failed");
    ASSERT(ts.tv_sec > 946684800, "time should be after Y2K");
    PASS();
}

static void test_clock_getres(void)
{
    struct timespec res;

    TEST(clock_getres);
    ASSERT(clock_getres(CLOCK_MONOTONIC, &res) == 0, "clock_getres failed");
    ASSERT(res.tv_sec == 0 && res.tv_nsec > 0, "resolution should be sub-second");
    PASS();
}

/* ===== math tests ===== */
static void test_round(void)
{
    TEST(round);
    ASSERT(round(1.4) == 1.0, "round(1.4) != 1.0");
    ASSERT(round(1.5) == 2.0, "round(1.5) != 2.0");
    ASSERT(round(-1.5) == -2.0, "round(-1.5) != -2.0");
    PASS();
}

static void test_trunc(void)
{
    TEST(trunc);
    ASSERT(trunc(1.9) == 1.0, "trunc(1.9) != 1.0");
    ASSERT(trunc(-1.9) == -1.0, "trunc(-1.9) != -1.0");
    PASS();
}

static void test_log2_exp2(void)
{
    TEST(log2_exp2);
    ASSERT(fabs(log2(8.0) - 3.0) < 0.0001, "log2(8) != 3");
    ASSERT(fabs(exp2(3.0) - 8.0) < 0.0001, "exp2(3) != 8");
    PASS();
}

static void test_fmin_fmax(void)
{
    TEST(fmin_fmax);
    ASSERT(fmin(1.0, 2.0) == 1.0, "fmin wrong");
    ASSERT(fmax(1.0, 2.0) == 2.0, "fmax wrong");
    PASS();
}

/* ===== memory tests ===== */
static void test_posix_memalign(void)
{
    void *ptr = NULL;

    TEST(posix_memalign);
    ASSERT(posix_memalign(&ptr, 64, 256) == 0, "posix_memalign failed");
    ASSERT(ptr != NULL, "returned NULL");
    ASSERT(((unsigned long)ptr & 63) == 0, "not aligned to 64");
    free(ptr);
    PASS();
}

static void test_reallocarray(void)
{
    void *ptr;

    TEST(reallocarray);
    ptr = reallocarray(NULL, 100, sizeof(int));
    ASSERT(ptr != NULL, "returned NULL");
    free(ptr);

    /* Overflow test */
    ptr = reallocarray(NULL, (size_t)-1, (size_t)-1);
    ASSERT(ptr == NULL, "should fail on overflow");
    ASSERT(errno == ENOMEM, "should set ENOMEM");
    PASS();
}

/* ===== filesystem tests ===== */
static void test_flock(void)
{
    int fd;

    TEST(flock);
    fd = open("/tmp/solcompat_flock_test", O_CREAT | O_WRONLY, 0644);
    ASSERT(fd >= 0, "open failed");
    ASSERT(flock(fd, LOCK_EX | LOCK_NB) == 0, "exclusive lock failed");
    ASSERT(flock(fd, LOCK_UN) == 0, "unlock failed");
    close(fd);
    unlink("/tmp/solcompat_flock_test");
    PASS();
}

static void test_scandir(void)
{
    struct dirent **namelist;
    int n;

    TEST(scandir);
    n = scandir("/tmp", &namelist, NULL, alphasort);
    ASSERT(n >= 0, "scandir failed");
    /* Free results */
    {
        int i;
        for (i = 0; i < n; i++)
            free(namelist[i]);
        free(namelist);
    }
    PASS();
}

/* ===== at_funcs tests ===== */
static void test_openat(void)
{
    int fd;
    char path[] = "/tmp/solcompat_openat_test";

    TEST(openat);
    /* AT_FDCWD should behave like normal open */
    fd = openat(AT_FDCWD, path, O_CREAT | O_WRONLY, 0644);
    ASSERT(fd >= 0, "openat(AT_FDCWD) failed");
    close(fd);
    unlink(path);
    PASS();
}

static void test_fstatat(void)
{
    struct stat st;

    TEST(fstatat);
    ASSERT(fstatat(AT_FDCWD, "/tmp", &st, 0) == 0, "fstatat /tmp failed");
    ASSERT(S_ISDIR(st.st_mode), "/tmp should be a directory");
    PASS();
}

/* ===== pipe2/dup3 tests ===== */
static void test_pipe2(void)
{
    int pipefd[2];

    TEST(pipe2);
    ASSERT(pipe2(pipefd, 0) == 0, "pipe2 failed");
    close(pipefd[0]);
    close(pipefd[1]);
    PASS();
}

static void test_dup3(void)
{
    int fd, newfd;

    TEST(dup3);
    fd = open("/dev/null", O_RDONLY);
    ASSERT(fd >= 0, "open /dev/null failed");
    newfd = dup3(fd, fd + 10, O_CLOEXEC);
    ASSERT(newfd == fd + 10, "dup3 wrong fd");
    close(fd);
    close(newfd);
    PASS();
}

/* ===== random tests ===== */
static void test_getentropy(void)
{
    unsigned char buf[32];

    TEST(getentropy);
    memset(buf, 0, sizeof(buf));
    ASSERT(getentropy(buf, sizeof(buf)) == 0, "getentropy failed");
    /* Extremely unlikely to be all zeros if working */
    {
        int all_zero = 1;
        int i;
        for (i = 0; i < 32; i++) {
            if (buf[i] != 0) { all_zero = 0; break; }
        }
        ASSERT(!all_zero, "all zeros — probably broken");
    }
    PASS();
}

static void test_arc4random(void)
{
    uint32_t a, b;

    TEST(arc4random);
    a = arc4random();
    b = arc4random();
    /* They could theoretically be equal but astronomically unlikely */
    ASSERT(a != b || a != arc4random(), "all same values — broken");
    PASS();
}

static void test_arc4random_uniform(void)
{
    int i;
    int bad = 0;

    TEST(arc4random_uniform);
    for (i = 0; i < 1000; i++) {
        uint32_t v = arc4random_uniform(100);
        if (v >= 100) { bad = 1; break; }
    }
    ASSERT(!bad, "value out of range");
    PASS();
}

/* ===== explicit_bzero test ===== */
static void test_explicit_bzero(void)
{
    char buf[16];

    TEST(explicit_bzero);
    memset(buf, 0xAA, sizeof(buf));
    explicit_bzero(buf, sizeof(buf));
    {
        int i, all_zero = 1;
        for (i = 0; i < 16; i++) {
            if (buf[i] != 0) { all_zero = 0; break; }
        }
        ASSERT(all_zero, "not zeroed");
    }
    PASS();
}

/* ===== ppoll test ===== */
static void test_ppoll(void)
{
    struct pollfd pfd;
    struct timespec ts;
    int ret;

    TEST(ppoll);
    pfd.fd = 0; /* stdin */
    pfd.events = POLLIN;
    pfd.revents = 0;

    ts.tv_sec = 0;
    ts.tv_nsec = 1000000; /* 1ms timeout */

    ret = ppoll(&pfd, 1, &ts, NULL);
    /* Should timeout (return 0) or succeed — not error */
    ASSERT(ret >= 0 || errno == EINTR, "ppoll failed unexpectedly");
    PASS();
}

/* ===== locale stub tests ===== */
static void test_locale_stubs(void)
{
    locale_t loc;

    TEST(locale_stubs);
    loc = newlocale(LC_ALL_MASK, "C", NULL);
    ASSERT(loc != NULL, "newlocale returned NULL");

    /* uselocale should return previous */
    uselocale(loc);
    freelocale(loc);
    PASS();
}

/* ===== IPv6 / Network tests ===== */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static void test_ipv6_types(void)
{
    struct sockaddr_in6 sin6;
    struct sockaddr_storage ss;
    struct in6_addr addr;

    TEST(ipv6_types);

    /* AF_INET6 should be 26 on Solaris */
    ASSERT(AF_INET6 == 26, "AF_INET6 should be 26");
    ASSERT(PF_INET6 == AF_INET6, "PF_INET6 should equal AF_INET6");

    /* sockaddr_in6 should be usable */
    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(80);
    sin6.sin6_scope_id = 0;
    ASSERT(sin6.sin6_family == AF_INET6, "sin6_family not set");

    /* sockaddr_storage should fit both v4 and v6 */
    ASSERT(sizeof(ss) >= sizeof(struct sockaddr_in), "ss too small for v4");
    ASSERT(sizeof(ss) >= sizeof(struct sockaddr_in6), "ss too small for v6");

    /* in6_addr should be 16 bytes */
    ASSERT(sizeof(addr) == 16, "in6_addr should be 16 bytes");

    PASS();
}

static void test_in6_macros(void)
{
    struct in6_addr any = IN6ADDR_ANY_INIT;
    struct in6_addr lo  = IN6ADDR_LOOPBACK_INIT;
    unsigned char mcast[16] = {0xff, 0x02, 0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    unsigned char ll[16]    = {0xfe, 0x80, 0,0,0,0,0,0,0,0,0,0,0,0,0,1};

    TEST(in6_macros);

    ASSERT(IN6_IS_ADDR_UNSPECIFIED(&any), ":: should be unspecified");
    ASSERT(!IN6_IS_ADDR_UNSPECIFIED(&lo), "::1 should not be unspecified");

    ASSERT(IN6_IS_ADDR_LOOPBACK(&lo), "::1 should be loopback");
    ASSERT(!IN6_IS_ADDR_LOOPBACK(&any), ":: should not be loopback");

    ASSERT(IN6_IS_ADDR_MULTICAST(mcast), "ff02::1 should be multicast");
    ASSERT(!IN6_IS_ADDR_MULTICAST(&lo), "::1 should not be multicast");

    ASSERT(IN6_IS_ADDR_LINKLOCAL(ll), "fe80::1 should be link-local");
    ASSERT(!IN6_IS_ADDR_LINKLOCAL(&lo), "::1 should not be link-local");

    /* in6addr_any/loopback globals should match */
    ASSERT(IN6_IS_ADDR_UNSPECIFIED(&in6addr_any), "in6addr_any should be ::");
    ASSERT(IN6_IS_ADDR_LOOPBACK(&in6addr_loopback), "in6addr_loopback should be ::1");

    PASS();
}

static void test_inet_ntop_v6(void)
{
    char buf[INET6_ADDRSTRLEN];
    struct in6_addr lo = IN6ADDR_LOOPBACK_INIT;
    struct in6_addr any = IN6ADDR_ANY_INIT;
    const char *result;

    TEST(inet_ntop_v6);

    result = inet_ntop(AF_INET6, &lo, buf, sizeof(buf));
    ASSERT(result != NULL, "inet_ntop(::1) returned NULL");
    /* With :: compression, ::1 should be exactly "::1" */
    ASSERT(strcmp(buf, "::1") == 0, "inet_ntop(::1) should produce '::1'");

    result = inet_ntop(AF_INET6, &any, buf, sizeof(buf));
    ASSERT(result != NULL, "inet_ntop(::) returned NULL");
    ASSERT(strcmp(buf, "::") == 0, "inet_ntop(::) should produce '::'");

    /* IPv4 should still work */
    {
        struct in_addr v4;
        v4.s_addr = htonl(0x7f000001); /* 127.0.0.1 */
        result = inet_ntop(AF_INET, &v4, buf, sizeof(buf));
        ASSERT(result != NULL, "inet_ntop(127.0.0.1) returned NULL");
        ASSERT(strcmp(buf, "127.0.0.1") == 0, "wrong v4 output");
    }

    /* Non-trivial address: 2001:db8::1 */
    {
        struct in6_addr a;
        memset(&a, 0, sizeof(a));
        a.s6_addr[0] = 0x20; a.s6_addr[1] = 0x01;
        a.s6_addr[2] = 0x0d; a.s6_addr[3] = 0xb8;
        a.s6_addr[15] = 0x01;
        result = inet_ntop(AF_INET6, &a, buf, sizeof(buf));
        ASSERT(result != NULL, "inet_ntop(2001:db8::1) returned NULL");
        ASSERT(strcmp(buf, "2001:db8::1") == 0, "expected '2001:db8::1'");
    }

    /* Address with no compressible run: fe80::1:2:3:4 */
    {
        struct in6_addr a;
        memset(&a, 0, sizeof(a));
        a.s6_addr[0] = 0xfe; a.s6_addr[1] = 0x80;
        a.s6_addr[8] = 0x00; a.s6_addr[9] = 0x01;
        a.s6_addr[10] = 0x00; a.s6_addr[11] = 0x02;
        a.s6_addr[12] = 0x00; a.s6_addr[13] = 0x03;
        a.s6_addr[14] = 0x00; a.s6_addr[15] = 0x04;
        result = inet_ntop(AF_INET6, &a, buf, sizeof(buf));
        ASSERT(result != NULL, "inet_ntop(fe80::1:2:3:4) returned NULL");
        ASSERT(strcmp(buf, "fe80::1:2:3:4") == 0, "expected 'fe80::1:2:3:4'");
    }
    PASS();
}

static void test_inet_pton_v6(void)
{
    struct in6_addr addr;
    int rc;

    TEST(inet_pton_v6);

    /* Full address */
    rc = inet_pton(AF_INET6, "2001:db8:85a3:0:0:8a2e:370:7334", &addr);
    ASSERT(rc == 1, "failed to parse full IPv6");
    ASSERT(addr.s6_addr[0] == 0x20 && addr.s6_addr[1] == 0x01, "wrong first bytes");

    /* :: abbreviation */
    rc = inet_pton(AF_INET6, "::", &addr);
    ASSERT(rc == 1, "failed to parse ::");
    ASSERT(IN6_IS_ADDR_UNSPECIFIED(&addr), ":: should be unspecified");

    /* ::1 */
    rc = inet_pton(AF_INET6, "::1", &addr);
    ASSERT(rc == 1, "failed to parse ::1");
    ASSERT(IN6_IS_ADDR_LOOPBACK(&addr), "::1 should be loopback");

    /* IPv4 should still work */
    {
        struct in_addr v4;
        rc = inet_pton(AF_INET, "192.168.1.1", &v4);
        ASSERT(rc == 1, "failed to parse IPv4");
    }

    PASS();
}

static void test_getaddrinfo_basic(void)
{
    struct addrinfo hints, *res = NULL;
    int rc;

    TEST(getaddrinfo_basic);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    /* NULL node + AI_PASSIVE should give INADDR_ANY */
    rc = getaddrinfo(NULL, "80", &hints, &res);
    ASSERT(rc == 0, "getaddrinfo(NULL, 80) failed");
    ASSERT(res != NULL, "no results");
    ASSERT(res->ai_family == AF_INET, "wrong family");
    ASSERT(res->ai_addrlen == sizeof(struct sockaddr_in), "wrong addrlen");

    {
        struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
        ASSERT(sin->sin_port == htons(80), "wrong port");
        ASSERT(sin->sin_addr.s_addr == INADDR_ANY, "should be INADDR_ANY");
    }

    freeaddrinfo(res);
    PASS();
}

static void test_getaddrinfo_ipv6_returns_noname(void)
{
    struct addrinfo hints, *res = NULL;
    int rc;

    TEST(getaddrinfo_ipv6_noname);

    /* AF_INET6 specifically requested — our impl honestly returns EAI_NONAME */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo("localhost", "80", &hints, &res);
    ASSERT(rc == EAI_NONAME, "should return EAI_NONAME for IPv6-only");
    PASS();
}

static void test_gai_strerror(void)
{
    TEST(gai_strerror);
    ASSERT(gai_strerror(0) != NULL, "gai_strerror(0) returned NULL");
    ASSERT(gai_strerror(EAI_NONAME) != NULL, "gai_strerror(EAI_NONAME) returned NULL");
    ASSERT(strlen(gai_strerror(EAI_MEMORY)) > 0, "empty error string");
    PASS();
}

static void test_ipv6_sockopts_defined(void)
{
    TEST(ipv6_sockopts_defined);
    /* Just verify the constants are defined and reasonable */
    ASSERT(IPV6_UNICAST_HOPS > 0, "IPV6_UNICAST_HOPS should be > 0");
    ASSERT(IPV6_MULTICAST_HOPS > 0, "IPV6_MULTICAST_HOPS should be > 0");
    ASSERT(IPV6_V6ONLY > 0, "IPV6_V6ONLY should be > 0");
    ASSERT(IPPROTO_IPV6 == 41, "IPPROTO_IPV6 should be 41");
    ASSERT(IPV6_JOIN_GROUP == IPV6_ADD_MEMBERSHIP, "alias mismatch");
    PASS();
}

/* ===== qsort_r tests ===== */

static int compare_ints_ascending(const void *left_element,
                                  const void *right_element,
                                  void *context_arg)
{
    int left_value  = *(const int *)left_element;
    int right_value = *(const int *)right_element;
    (void)context_arg;
    return left_value - right_value;
}

static int compare_ints_with_direction(const void *left_element,
                                       const void *right_element,
                                       void *direction_flag)
{
    int left_value  = *(const int *)left_element;
    int right_value = *(const int *)right_element;
    int multiplier  = *(int *)direction_flag;
    return multiplier * (left_value - right_value);
}

static void test_qsort_r_ascending(void)
{
    int values[] = { 5, 3, 8, 1, 9, 2, 7, 4, 6 };
    int element_count = 9;
    int context_unused = 0;
    int index;

    TEST(qsort_r_ascending);
    qsort_r(values, (size_t)element_count, sizeof(int),
            compare_ints_ascending, &context_unused);
    for (index = 0; index < element_count - 1; index++)
        ASSERT(values[index] <= values[index + 1], "not sorted ascending");
    PASS();
}

static void test_qsort_r_descending(void)
{
    int values[] = { 5, 3, 8, 1, 9, 2, 7, 4, 6 };
    int element_count = 9;
    int direction = -1;   /* negative multiplier → descending */
    int index;

    TEST(qsort_r_descending);
    qsort_r(values, (size_t)element_count, sizeof(int),
            compare_ints_with_direction, &direction);
    for (index = 0; index < element_count - 1; index++)
        ASSERT(values[index] >= values[index + 1], "not sorted descending");
    PASS();
}

static void test_qsort_r_large(void)
{
    /* Sort an array larger than the insertion-sort cutoff (12 elements)
     * to exercise the quicksort path.                                   */
    int values[64];
    int expected[64];
    int element_count = 64;
    int context_unused = 0;
    int index;

    TEST(qsort_r_large);
    for (index = 0; index < element_count; index++)
        values[index] = element_count - index;   /* reverse-sorted input */
    for (index = 0; index < element_count; index++)
        expected[index] = index + 1;

    qsort_r(values, (size_t)element_count, sizeof(int),
            compare_ints_ascending, &context_unused);
    for (index = 0; index < element_count; index++)
        ASSERT(values[index] == expected[index], "wrong value after sort");
    PASS();
}

static void test_qsort_r_single_element(void)
{
    int single_value = 42;
    int context_unused = 0;

    TEST(qsort_r_single_element);
    qsort_r(&single_value, 1, sizeof(int),
            compare_ints_ascending, &context_unused);
    ASSERT(single_value == 42, "single element changed");
    PASS();
}

/* ===== execvpe tests ===== */

static void test_execvpe_direct_path(void)
{
    pid_t child_pid;
    int   wait_status;
    char *child_argv[] = { "/bin/true", NULL };
    char *child_env[]  = { "PATH=/bin:/usr/bin", NULL };

    TEST(execvpe_direct_path);
    child_pid = fork();
    ASSERT(child_pid >= 0, "fork failed");

    if (child_pid == 0) {
        /* Child: execute /bin/true directly (contains a slash — no search) */
        execvpe("/bin/true", child_argv, child_env);
        _exit(127);
    }

    waitpid(child_pid, &wait_status, 0);
    ASSERT(WIFEXITED(wait_status), "child did not exit normally");
    ASSERT(WEXITSTATUS(wait_status) == 0, "exit status not 0");
    PASS();
}

static void test_execvpe_path_search(void)
{
    pid_t  child_pid;
    int    wait_status;
    char  *child_argv[] = { "true", NULL };
    char  *child_env[]  = { "PATH=/bin:/usr/bin", NULL };

    TEST(execvpe_path_search);
    child_pid = fork();
    ASSERT(child_pid >= 0, "fork failed");

    if (child_pid == 0) {
        /* Child: search for "true" via PATH in the provided environment */
        execvpe("true", child_argv, child_env);
        _exit(127);
    }

    waitpid(child_pid, &wait_status, 0);
    ASSERT(WIFEXITED(wait_status), "child did not exit normally");
    ASSERT(WEXITSTATUS(wait_status) == 0, "exit status not 0");
    PASS();
}

static void test_execvpe_not_found(void)
{
    pid_t child_pid;
    int   wait_status;
    char *child_argv[] = { "nonexistent_command_xyzzy", NULL };
    char *child_env[]  = { "PATH=/bin:/usr/bin", NULL };

    TEST(execvpe_not_found);
    child_pid = fork();
    ASSERT(child_pid >= 0, "fork failed");

    if (child_pid == 0) {
        execvpe("nonexistent_command_xyzzy", child_argv, child_env);
        _exit(127);   /* expected: command not found */
    }

    waitpid(child_pid, &wait_status, 0);
    ASSERT(WIFEXITED(wait_status), "child did not exit normally");
    ASSERT(WEXITSTATUS(wait_status) == 127, "should exit 127 for not-found");
    PASS();
}

/* ===== dirfd tests ===== */

static void test_dirfd_valid(void)
{
    DIR        *dir_stream;
    int         returned_fd;
    struct stat fd_stat;

    TEST(dirfd_valid);
    dir_stream = opendir("/tmp");
    ASSERT(dir_stream != NULL, "opendir /tmp failed");

    returned_fd = dirfd(dir_stream);
    ASSERT(returned_fd >= 0, "dirfd returned negative value");

    /* The fd must be a valid, open file descriptor */
    ASSERT(fstat(returned_fd, &fd_stat) == 0, "fstat on dirfd failed");
    ASSERT(S_ISDIR(fd_stat.st_mode), "fd is not a directory");

    closedir(dir_stream);
    PASS();
}

static void test_dirfd_null(void)
{
    int result;

    TEST(dirfd_null);
    result = dirfd(NULL);
    ASSERT(result == -1, "dirfd(NULL) should return -1");
    ASSERT(errno == EINVAL, "dirfd(NULL) should set EINVAL");
    PASS();
}

/* ===== Main ===== */
int
main(void)
{
    printf("libsolcompat test suite v%s\n", SOLCOMPAT_VERSION_STRING);
    printf("========================================\n\n");

    printf("[snprintf]\n");
    test_snprintf_basic();
    test_snprintf_truncation();
    test_snprintf_null_measure();

    printf("\n[string]\n");
    test_strndup();
    test_strnlen();
    test_strlcpy();
    test_strlcat();
    test_strcasestr();
    test_memmem();
    test_strsep();
    test_memrchr();

    printf("\n[stdio]\n");
    test_vasprintf_wrapper();
    test_getline();

    printf("\n[stdlib]\n");
    test_setenv_unsetenv();
    test_mkdtemp();
    test_qsort_r_ascending();
    test_qsort_r_descending();
    test_qsort_r_large();
    test_qsort_r_single_element();

    printf("\n[clock]\n");
    test_clock_monotonic();
    test_clock_realtime();
    test_clock_getres();

    printf("\n[math]\n");
    test_round();
    test_trunc();
    test_log2_exp2();
    test_fmin_fmax();

    printf("\n[memory]\n");
    test_posix_memalign();
    test_reallocarray();

    printf("\n[filesystem]\n");
    test_flock();
    test_scandir();
    test_dirfd_valid();
    test_dirfd_null();

    printf("\n[at_funcs]\n");
    test_openat();
    test_fstatat();

    printf("\n[process]\n");
    test_pipe2();
    test_dup3();
    test_execvpe_direct_path();
    test_execvpe_path_search();
    test_execvpe_not_found();

    printf("\n[random]\n");
    test_getentropy();
    test_arc4random();
    test_arc4random_uniform();
    test_explicit_bzero();

    printf("\n[poll]\n");
    test_ppoll();

    printf("\n[stubs]\n");
    test_locale_stubs();

    printf("\n[ipv6/network]\n");
    test_ipv6_types();
    test_in6_macros();
    test_inet_ntop_v6();
    test_inet_pton_v6();
    test_getaddrinfo_basic();
    test_getaddrinfo_ipv6_returns_noname();
    test_gai_strerror();
    test_ipv6_sockopts_defined();

    printf("\n========================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}
