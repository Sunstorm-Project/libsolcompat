/* GNU mount-table access over the native Solaris two-argument ABI.
 * These prefixed exports never interpose native getmntent/hasmntopt. */
#include <stdarg.h>
#include <stdio.h>
#include <sys/mnttab.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <mntent.h>
#undef getmntent
#undef hasmntopt

FILE *solcompat_setmntent(const char *path, const char *mode)
{
    char native_mode[8]; size_t i=0; int cloexec=0; FILE *fp;
    for (;*mode;mode++) {
        if (*mode=='e') cloexec=1;
        else if (i+1<sizeof native_mode) native_mode[i++]=*mode;
        else { errno=EINVAL; return NULL; }
    }
    native_mode[i]=0;
    fp=fopen(path,native_mode);
    if (fp && cloexec && fcntl(fileno(fp),F_SETFD,FD_CLOEXEC)<0) {
        int saved=errno; fclose(fp);errno=saved;return NULL;
    }
    return fp;
}
/* Native Solaris leaves mount-table escapes literal. GNU getmntent
 * decodes these four sequences exactly once; do not rescan decoded output. */
static char *decode_mount_field(char *field)
{
    char *in=field,*out=field;
    if(!field)return NULL;
    while(*in) {
        if(!strncmp(in,"\\040",4)) {*out++=' ';in+=4;}
        else if(!strncmp(in,"\\011",4)) {*out++='\t';in+=4;}
        else if(!strncmp(in,"\\012",4)) {*out++='\n';in+=4;}
        else if(!strncmp(in,"\\134",4)) {*out++='\\';in+=4;}
        else *out++=*in++;
    }
    *out=0;
    return field;
}

struct mntent *solcompat_getmntent(FILE *fp)
{
    static struct mnttab native;
    static struct mntent entry;
    int ret;
    if (!fp) { errno=EINVAL;return NULL; }
    ret=getmntent(fp,&native);
    if(ret) {if(ret>0) errno=EINVAL;return NULL;}
    entry.mnt_fsname=decode_mount_field(native.mnt_special);
    entry.mnt_dir=decode_mount_field(native.mnt_mountp);
    entry.mnt_type=decode_mount_field(native.mnt_fstype);
    entry.mnt_opts=decode_mount_field(native.mnt_mntopts);
    entry.mnt_freq=0;entry.mnt_passno=0;
    return &entry;
}
int solcompat_endmntent(FILE *fp)
{
    if(fp) fclose(fp);
    return 1;
}
char *solcompat_hasmntopt(const struct mntent *entry,const char *option)
{
    char *p;size_t n;
    if(!entry || !entry->mnt_opts || !option || !*option) return NULL;
    n=strlen(option);p=entry->mnt_opts;
    while(*p) {
        if(!strncmp(p,option,n) && (p[n]==0 || p[n]==',' || p[n]=='=')) return p;
        p=strchr(p,',');if(!p) break;p++;
    }
    return NULL;
}
static int field(FILE *fp,const char *s)
{
    for(;*s;s++) {
        const char *escape=NULL;
        if(*s==' ')escape="\\040";else if(*s=='\t')escape="\\011";
        else if(*s=='\n')escape="\\012";else if(*s=='\\')escape="\\134";
        if(escape) {if(fputs(escape,fp)==EOF)return -1;}
        else if(fputc((unsigned char)*s,fp)==EOF)return -1;
    }
    return 0;
}
int solcompat_addmntent(FILE *fp,const struct mntent *entry)
{
    const char *fields[5];int i;
    if(!fp || !entry) {errno=EINVAL;return 1;}
    fields[0]=entry->mnt_fsname;fields[1]=entry->mnt_dir;
    fields[2]=entry->mnt_type;fields[3]=entry->mnt_opts;fields[4]="0";
    if(fseek(fp,0,SEEK_END)<0)return 1;
    for(i=0;i<5;i++) {
        if(!fields[i] || field(fp,fields[i])<0)return 1;
        if(fputc(i==4?'\n':'\t',fp)==EOF)return 1;
    }
    return 0;
}
/* Solaris has no native symbols with these names. Export the GNU names too
 * so link-only feature checks agree with the advertised compatibility API. */
#undef setmntent
#undef endmntent
#undef addmntent
FILE *setmntent(const char *path,const char *mode)
{ return solcompat_setmntent(path,mode); }
int endmntent(FILE *fp)
{ return solcompat_endmntent(fp); }
int addmntent(FILE *fp,const struct mntent *entry)
{ return solcompat_addmntent(fp,entry); }
