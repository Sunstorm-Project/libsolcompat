#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
extern int solcompat_snprintf(char*,size_t,const char*,...);
int main(void) {
 int e;long double m;char text[80];
 m=frexpl(0.75L,&e);if(m!=0.75L||e!=0)return 1;
 m=frexpl(-8.0L,&e);if(m!=-0.5L||e!=4)return 2;
 m=frexpl(LDBL_MIN/2,&e);if(m!=0.5L||e!=LDBL_MIN_EXP-1)return 3;
 m=frexpl(-0.0L,&e);if(m!=0||!__builtin_signbit(m)||e!=0)return 4;
#if LDBL_MAX_EXP > 2000
 m=frexpl(0x1p2000L,&e);if(m!=0.5L||e!=2001)return 5;
 m=frexpl(0x1p-2000L,&e);if(m!=0.5L||e!=-1999)return 6;
 if(solcompat_snprintf(text,sizeof text,"%La",0x1p2000L)!=9||strcmp(text,"0x1p+2000"))return 7;
 if(solcompat_snprintf(text,sizeof text,"%La",0x1p-2000L)!=9||strcmp(text,"0x1p-2000"))return 8;
#endif
 write(1,"long-double vectors passed\n",sizeof("long-double vectors passed\n")-1);return 0;
}
