#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <solcompat/at_funcs.h>

int main(void)
{
    struct { struct stat value; unsigned char guard[64]; } actual;
    struct stat expected;
    unsigned char guard[64];
    memset(&actual, 0xa5, sizeof(actual));
    memset(guard, 0xa5, sizeof(guard));
    if (stat("/tmp", &expected) || fstatat(AT_FDCWD, "/tmp", &actual.value, 0))
        return 1;
    return actual.value.st_mode != expected.st_mode ||
        actual.value.st_ino != expected.st_ino ||
        actual.value.st_size != expected.st_size ||
        memcmp(actual.guard, guard, sizeof(guard)) != 0;
}
