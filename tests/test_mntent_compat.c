#include <stdio.h>
#include <mntent.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
int main(void) {
 FILE *f;struct mntent *e;int count=0;
 errno=0;if(getmntent(NULL)!=NULL||errno!=EINVAL)return 1;
 f=setmntent("/etc/mnttab","re");if(!f)return 2;
 if(!(fcntl(fileno(f),F_GETFD)&FD_CLOEXEC))return 3;
 while((e=getmntent(f))) {if(!e->mnt_fsname||!e->mnt_dir||!e->mnt_type||!e->mnt_opts)return 4;count++;}
 if(endmntent(f)!=1||!count)return 5;
 {struct mntent v={"device","/test","ufs","root,rw,nosuid=1",0,0};
 if(hasmntopt(&v,"ro"))return 6;
 if(!hasmntopt(&v,"rw")||!hasmntopt(&v,"nosuid"))return 7;
 f=tmpfile();if(!f)return 8;if(addmntent(f,&v))return 9;rewind(f);
 e=getmntent(f);if(!e||strcmp(e->mnt_fsname,"device")||strcmp(e->mnt_dir,"/test"))return 10;
 endmntent(f);}
 {const char *names[]={"/a b","/a\\b","/a\tb","/a\\040b"};unsigned i;
  for(i=0;i<sizeof(names)/sizeof(names[0]);i++) {
   struct mntent in={"device",(char*)names[i],"ufs","rw",0,0};
   f=tmpfile();if(!f)return 11;if(addmntent(f,&in))return 12;rewind(f);
   e=getmntent(f);if(!e||strcmp(e->mnt_dir,names[i]))return 13;
   endmntent(f);
  }
 }
 puts("GNU mount bridge vectors passed");return 0;
}
