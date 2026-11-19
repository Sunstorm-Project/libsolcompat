/* Exact binary decomposition without narrowing Solaris SPARC binary128
 * long double to double. Power-of-two scaling is exact, including subnormals. */
#include <float.h>
#include <limits.h>

long double frexpl(long double value, int *exponent)
{
    long double factors[sizeof(int)*CHAR_BIT];
    int powers[sizeof(int)*CHAR_BIT];
    int count=1, i, negative;
    *exponent=0;
    if (value==0 || value!=value || value==__builtin_infl() || value==-__builtin_infl())
        return value;
    negative=value<0;
    if(negative)value=-value;
    factors[0]=2; powers[0]=1;
    while(powers[count-1] <= (LDBL_MAX_EXP-1)/2) {
        factors[count]=factors[count-1]*factors[count-1];
        powers[count]=powers[count-1]*2;count++;
    }
    if(value>=1) {
        for(i=count-1;i>=0;i--) {
            if(value>=factors[i]) {value/=factors[i];*exponent+=powers[i];}
        }
        value*=0.5L;(*exponent)++;
    } else {
        while(value<0.5L) {
            for(i=count-1;i>=0;i--) {
                if(value<1.0L/factors[i]) {value*=factors[i];*exponent-=powers[i];}
            }
            if(value<0.5L) {value*=2;(*exponent)--;}
        }
    }
    return negative?-value:value;
}
