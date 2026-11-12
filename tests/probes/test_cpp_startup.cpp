#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <unistd.h>
static int count;
struct Global {
 Global(){++count;}
 ~Global(){if(count==1)write(1,"global destructor passed\n",sizeof("global destructor passed\n")-1);else _exit(9);}
} global;
static int local_count;
struct Local {Local(){++local_count;}};
static void local(){static Local l;}
int main(){
 {char status[]="constructor count=0\n";status[18]='0'+count;write(1,status,sizeof(status)-1);}
 if(count!=1)return 1;local();local();if(local_count!=1)return 2;
 try{throw std::runtime_error("exception sentinel");}
 catch(const std::runtime_error &e){if(std::strcmp(e.what(),"exception sentinel"))return 3;}
 catch(...){return 4;}
 char b[40];if(std::snprintf(b,sizeof b,"%lld",4294967297LL)!=10)return 5;
 if(std::strcmp(b,"4294967297"))return 6;
 write(1,"C++ startup and exceptions passed\n",sizeof("C++ startup and exceptions passed\n")-1);return 0;
}
