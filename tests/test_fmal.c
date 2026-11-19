#include <float.h>
#include <math.h>
#include <stdio.h>
#include <fenv.h>
int main(void)
{
    long double x = 1.0L + 0x1p-57L;
    if (fmal(x, 1.0L - 0x1p-57L, -1.0L) != -0x1p-114L ||
        fmal(LDBL_MAX, 2, -LDBL_MAX) != LDBL_MAX ||
        fmal(0x1p2000L, 0x1p-1000L, 1) != 0x1p1000L ||
        fmal(LDBL_MIN, 0.5L, -LDBL_MIN*LDBL_EPSILON) !=
            LDBL_MIN/2 - LDBL_MIN*LDBL_EPSILON ||
        ilogbl(0x1p-2000L) != -2000 ||
        !signbit(fmal(-0.0L, 1, -0.0L)))
        return 1;
    if (fesetround(FE_UPWARD) || fmal(0x1p-300L, 0x1p-300L, 1) !=
            1.0L + LDBL_EPSILON || fegetround() != FE_UPWARD ||
            fesetround(FE_TONEAREST))
        return 2;
    puts("PASS long-double fused arithmetic and directed rounding");
    return 0;
}
