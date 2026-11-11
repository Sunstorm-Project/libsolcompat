// Model the false native-prototype claims from a bootstrap C++ facade.
#ifndef __CORRECT_ISO_CPP11_MATH_H_PROTO_FP
#define __CORRECT_ISO_CPP11_MATH_H_PROTO_FP 1
#endif
#ifndef __CORRECT_ISO_CPP11_MATH_H_PROTO_INT
#define __CORRECT_ISO_CPP11_MATH_H_PROTO_INT 1
#endif
#ifdef MATH_FIRST
#include <math.h>
#include <cmath>
#else
#include <cmath>
#include <math.h>
#endif
#include <cstdlib>
#include <type_traits>
static_assert(std::is_same<decltype(std::isnan(1.0)),bool>::value,"classification overload");
int main(){
 char *end=0;
 if(std::strtoll("123",&end,10)!=123 || *end) return 1;
 if(std::abs(-4)!=4 || std::sqrt(9.0)!=3.0) return 2;
 if(std::isnan(0)||!std::isfinite(7)) return 5;
 if(std::isnan(1.0)||std::isnan(1.0L)||!std::isfinite(1.0L)) return 3;
 if(!std::isnan(__builtin_nan(""))||!std::isnan(__builtin_nanl(""))) return 4;
 return std::round(2.4)!=2.0 || std::round(2.4L)!=2.0L;
}
