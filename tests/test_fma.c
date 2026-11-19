#include <float.h>
#include <math.h>
#include <stdio.h>

/* Force an external call even when GCC knows the inputs. */
static double (*volatile fused)(double, double, double) = fma;

int main(void)
{
    if (fused(1.0 + 0x1p-27, 1.0 - 0x1p-27, -1.0) != -0x1p-54) {
        fputs("cancellation failed\n", stderr);
        return 1;
    }
    if (fused(DBL_MAX, 2.0, -DBL_MAX) != DBL_MAX) {
        fputs("overflow cancellation failed\n", stderr);
        return 1;
    }
    if (fused(DBL_MIN, 0.5, -0x1p-1074) != 0x0.7ffffffffffffp-1022) {
        fputs("subnormal failed\n", stderr);
        return 1;
    }
    if (!signbit(fused(-0.0, 1.0, -0.0))) {
        fputs("negative zero failed\n", stderr);
        return 1;
    }
    if (!isnan(fused(INFINITY, 0.0, 1.0))) {
        fputs("invalid product failed\n", stderr);
        return 1;
    }
    puts("PASS fused multiply-add cancellation, overflow, subnormal and special values");
    return 0;
}
