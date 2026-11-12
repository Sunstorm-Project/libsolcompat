#include <stdexcept>
#include <cstring>
#include <unistd.h>
static int count, local_count;
#define TAG(s) write(1,s "\n",sizeof(s "\n")-1)
struct Global {Global(){++count;}~Global(){TAG("global destructor reached");}} global;
struct Local {Local(){++local_count;}};
static void local(){static Local l;}
int main(){
 char text[]="constructor count=0\n";text[18]='0'+count;write(1,text,sizeof(text)-1);
 if(count!=1)return 1;
 TAG("before local static");local();local();if(local_count!=1)return 2;TAG("local static passed");
 TAG("before throw int");try{throw 42;}catch(int value){if(value!=42)return 3;}catch(...){return 4;}TAG("throw int passed");
 TAG("before runtime_error");try{throw std::runtime_error("sentinel");}catch(const std::runtime_error &e){if(std::strcmp(e.what(),"sentinel"))return 5;}catch(...){return 6;}TAG("runtime_error passed");return 0;
}
