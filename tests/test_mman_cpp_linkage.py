#!/usr/bin/env python3
"""Host-only linkage regression using authored append and harmless C stubs."""
import os,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
HEADER=ROOT/'include/sysroot-prep/sys/mman.h.append'
PREFIX='#include <sys/types.h>\n#include <stddef.h>\ntypedef unsigned int uint_t;\n'
STUBS='''int posix_madvise(void *p,size_t n,int a){(void)p;(void)n;(void)a;return 0;}
int shm_open(const char *p,int f,mode_t m){(void)p;(void)f;(void)m;return 0;}
int shm_unlink(const char *p){(void)p;return 0;}
'''
CALL='int main(void){return posix_madvise(0,0,0)+shm_open("",0,0)+shm_unlink("");}\n'
class Linkage(unittest.TestCase):
 def check_mode(self,mode,outer=False):
  with tempfile.TemporaryDirectory() as tmp:
   t=Path(tmp);stub=t/'stub.c';stub.write_text(PREFIX+f'#include "{HEADER}"\n'+STUBS)
   subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c99','-c',str(stub),'-o',str(t/'stub.o')],check=True,capture_output=True)
   cpp='++' in mode;p=t/('caller.cc' if cpp else 'caller.c')
   body=f'#include "{HEADER}"\n'
   if outer:body='extern "C++" {\n'+body+'}\n'
   p.write_text(PREFIX+body+CALL)
   compiler=os.environ.get('HOST_CXX','c++') if cpp else os.environ.get('HOST_CC','cc')
   subprocess.run([compiler,'-std='+mode,str(p),str(t/'stub.o'),'-o',str(t/'probe')],check=True,capture_output=True)
   subprocess.run([str(t/'probe')],check=True)
 def test_c99_unchanged(self):self.check_mode('c99')
 def test_strict_cpp17(self):self.check_mode('c++17')
 def test_gnu_cpp17(self):self.check_mode('gnu++17')
 def test_outer_cpp_linkage(self):self.check_mode('c++17',True)
if __name__=='__main__':unittest.main()
