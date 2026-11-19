#include <float.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    long double direction = 1.0L + LDBL_EPSILON;
    if (nextafterf(1, 2) != 1 + FLT_EPSILON ||
        nextafterl(1, 2) != direction ||
        nextafterl(1, 0) != 1.0L - LDBL_EPSILON / 2 ||
        nexttoward(1, direction) != 1 + DBL_EPSILON ||
        nexttowardf(1, direction) != 1 + FLT_EPSILON ||
        nextafterl(0, -1) != -LDBL_MIN * LDBL_EPSILON ||
        !signbit(nextafterl(-LDBL_MIN * LDBL_EPSILON, 1)) ||
        nextafterl(INFINITY, 0) != LDBL_MAX ||
        !isinf(nextafterl(LDBL_MAX, INFINITY)) ||
        copysignl(direction, -0.0L) != -direction ||
        fabsl(-direction) != direction)
        return 1;
    puts("PASS adjacent floats, wide direction, signed zero and range boundaries");
    return 0;
}
