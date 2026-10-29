/* Regression for the heap overflow triggered by Willy Beamish's scene dump.
 * Run with both libc-first and libsolcompat-first dynamic dependency orders. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int solcompat_snprintf(char *, size_t, const char *, ...);

int main(void)
{
    const size_t lengths[] = {0, 8191, 8192, 8193, 65536, 262144};
    unsigned int i;
    char numeric[32];

    if (solcompat_snprintf(numeric, sizeof numeric, "%d", 7) != 1 ||
        strcmp(numeric, "7")) {
        puts("FAIL: integer formatting");
        return 4;
    }
    for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        size_t n = lengths[i];
        char *input = malloc(n + 1), *output = malloc(n + 3);
        char small[18];
        int result;

        if (!input || !output)
            return 5;
        memset(input, 'Q', n);
        input[n] = '\0';
        memset(small, 0x55, sizeof small);

        result = solcompat_snprintf(output, n + 3, "[%s]", input);
        if (result != (int)n + 2 || output[0] != '[' ||
            output[n + 1] != ']' || output[n + 2] != '\0' ||
            memcmp(output + 1, input, n))
            return 1;

        /* Truncation must preserve adjacent memory and report full length. */
        result = solcompat_snprintf(small + 1, 8, "[%s]", input);
        if (result != (int)n + 2 || small[0] != 0x55 ||
            small[9] != 0x55 || (n >= 7 && small[8] != '\0'))
            return 2;
        if (solcompat_snprintf(NULL, 0, "[%s]", input) != (int)n + 2)
            return 3;
        free(output);
        free(input);
    }
    puts("PASS: integer, long output, truncation canaries, length-only formatting");
    return 0;
}
