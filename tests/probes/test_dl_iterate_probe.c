#include <stdio.h>
#include <sys/link.h>
#include <unistd.h>
static int count;
static int callback(struct dl_phdr_info *info,size_t size,void *arg){
 char text[256];int n;(void)size;(void)arg;count++;
 n=snprintf(text,sizeof text,"object=%s phnum=%u\n",info->dlpi_name,(unsigned)info->dlpi_phnum);
 if(n>0)write(1,text,(size_t)n<sizeof text?(size_t)n:sizeof text-1);return 0;
}
int main(void){char text[80];int n;dl_iterate_phdr(callback,0);n=snprintf(text,sizeof text,"callback count=%d\n",count);write(1,text,n);return count?0:1;}
