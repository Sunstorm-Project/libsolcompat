#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    long double x = 1.0L + LDBL_EPSILON;
    if (scalbnl(x, 2010) != x * 0x1p2010L ||
        ldexpl(x, -2010) != x * 0x1p-2010L ||
        scalblnl(x, LONG_MIN) != 0 || !isinf(scalblnl(x, LONG_MAX)) ||
        scalbnl(LDBL_MIN, 1-LDBL_MANT_DIG) != LDBL_MIN * LDBL_EPSILON ||
        scalbnl(LDBL_MIN * LDBL_EPSILON, LDBL_MANT_DIG-1) != LDBL_MIN ||
        !signbit(ldexpl(-0.0L, 42)))
        return 1;
    puts("PASS long double scaling, subnormal, signed zero and extreme exponents");
    return 0;
}
