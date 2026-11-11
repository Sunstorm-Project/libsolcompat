#include <stdio.h>
#include <sys/link.h>
#include <stdint.h>
static int seen,bad;
int main(void);
static int inspect(struct dl_phdr_info *info,size_t size,void *arg)
{
 unsigned i;int contains_main=0;(void)size;(void)arg;
 if(seen++)return 0;
 for(i=0;i<info->dlpi_phnum;i++) {
  const Elf32_Phdr *p=&info->dlpi_phdr[i];uintptr_t address=info->dlpi_addr+p->p_vaddr;
  if(p->p_type==PT_LOAD && (uintptr_t)&main>=address && (uintptr_t)&main<address+p->p_memsz)contains_main=1;
  if(p->p_type==PT_PHDR && address!=(uintptr_t)info->dlpi_phdr)bad++;
 }
 if(!contains_main)bad++;
 return 0;
}
static int stop(struct dl_phdr_info *info,size_t size,void *arg)
{(void)info;(void)size;(void)arg;return 42;}
int main(void)
{
 if(dl_iterate_phdr(inspect,0)!=0 || !seen || bad)return 1;
 if(dl_iterate_phdr(stop,0)!=42)return 2;
 puts("program-header load-bias vectors passed");return 0;
}
