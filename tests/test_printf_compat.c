#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <limits.h>
#include <wchar.h>
#include <unistd.h>
extern int solcompat_snprintf(char *,size_t,const char *,...);
extern int solcompat_vsnprintf(char *,size_t,const char *,va_list);
extern int solcompat_fprintf(FILE *,const char *,...);
static int failures;
#define CHECK(x) do { if (!(x)) { const char m[]="FAIL line " #x "\n"; write(2,m,sizeof(m)-1); failures++; } } while(0)
static int vfmt(char *b,size_t n,const char *fmt,...) {va_list a;int r;va_start(a,fmt);r=solcompat_vsnprintf(b,n,fmt,a);va_end(a);return r;}
int main(void) {
 char b[256], small[6]={'X','X','X','X','Q','R'}; int n=-1; FILE *f;
 CHECK(solcompat_snprintf(b,sizeof b,"%ju %zu %td",(uintmax_t)4294967297ULL,(size_t)123,(ptrdiff_t)-7)==17);
 CHECK(!strcmp(b,"4294967297 123 -7"));
 CHECK(solcompat_snprintf(b,sizeof b,"%hhd %hhu %hd",257,511,65537)==7); CHECK(!strcmp(b,"1 255 1"));
 CHECK(solcompat_snprintf(b,sizeof b,"%2$*1$.*3$jd",8,(intmax_t)42,4)==8); CHECK(!strcmp(b,"    0042"));
 CHECK(solcompat_snprintf(b,sizeof b,"%*.*s",-6,3,"abcdef")==6); CHECK(!strcmp(b,"abc   "));
 CHECK(vfmt(b,sizeof b,"%.2f %.1Lf %a",1.25,(long double)2.5,1.0)>0); CHECK(!strcmp(b,"1.25 2.5 0x1p+0"));
 CHECK(solcompat_snprintf(NULL,0,"%020000d",1)==20000);
 CHECK(solcompat_snprintf(small,4,"abcdef")==6); CHECK(!memcmp(small,"abc\0QR",6));
 CHECK(solcompat_snprintf(b,sizeof b,"abc%nxyz",&n)==6); CHECK(n==3); CHECK(!strcmp(b,"abcxyz"));
 {char *fmt=malloc(12007),*out=malloc(12020);memset(fmt,'a',12000);strcpy(fmt+12000,"%ju");CHECK(solcompat_snprintf(out,12020,fmt,(uintmax_t)42)==12002);CHECK(!strcmp(out+12000,"42"));free(fmt);free(out);}
 {char *str=malloc(50001),*out=malloc(50001);memset(str,'b',50000);str[50000]=0;CHECK(solcompat_snprintf(out,50001,"%s",str)==50000);CHECK(!strcmp(str,out));free(str);free(out);}
 CHECK(solcompat_snprintf(b,sizeof b,"%ls",L"ASCII")==5);CHECK(!strcmp(b,"ASCII"));
 f=tmpfile();CHECK(f!=NULL);if(f){CHECK(solcompat_fprintf(f,"%ju:%hhu",(uintmax_t)4294967297ULL,511)==14);rewind(f);CHECK(fread(b,1,14,f)==14);CHECK(!memcmp(b,"4294967297:255",14));fclose(f);}
 /* Plain symbol callers, including function pointers, must use same engine. */
 {int (*p)(char *,size_t,const char *,...)=snprintf;CHECK(p(b,sizeof b,"%ju",(uintmax_t)4294967297ULL)==10);CHECK(!strcmp(b,"4294967297"));}
 if(!failures)write(1,"printf compatibility vectors passed\n",36);
 return failures?1:0;
}
