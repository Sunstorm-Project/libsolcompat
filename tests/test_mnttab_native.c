#include <stdio.h>
#include <sys/mnttab.h>
int main(void){FILE *f=fopen("/etc/mnttab","r");struct mnttab row;int n=0,r;
 if(!f)return 1;
 while((r=getmntent(f,&row))==0){if(!row.mnt_special||!row.mnt_mountp||!row.mnt_fstype)return 2;n++;}
 fclose(f);return !n||r>0?3:0;}
