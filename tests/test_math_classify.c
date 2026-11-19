#include <float.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    volatile float subnormal = 0x1p-149f;
    volatile float small = FLT_MIN;
    int calls = 0;
    if (isnormal(subnormal) || !isnormal((double)subnormal) ||
        !isnormal(small) || !signbit(-0.0L) || isnan(0) ||
        !isfinite(++calls) || calls != 1)
        return 1;
    puts("PASS float and double classification, signed zero and single evaluation");
    return 0;
}
