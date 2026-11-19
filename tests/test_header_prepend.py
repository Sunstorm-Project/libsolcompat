import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class HeaderPrepend(unittest.TestCase):
    def run_case(self, definition, valid):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d); prep=root/'include/sysroot-prep';prep.mkdir(parents=True)
            prepend='#ifndef _SOLCOMPAT_STDIO_H_PREPEND\n#define _SOLCOMPAT_STDIO_H_PREPEND\ntypedef void *test_va_list;\n#endif\n'
            (prep/'stdio.h.prepend').write_text(prepend)
            shutil.copyfile(ROOT/'Makefile',root/'Makefile')
            header=root/'sysroot/usr/include/stdio.h';header.parent.mkdir(parents=True)
            native='#ifndef _STDIO_H\n'+definition+'\n#if !defined(_NO_LONGLONG)\n#endif\nint native_declaration;\n#endif\n'
            header.write_text(native)
            def build():return subprocess.run(['make','install-headers','SYSROOT='+str(root/'sysroot')],cwd=root,capture_output=True,text=True)
            result=build()
            if valid:
                self.assertEqual(result.returncode,0,result.stderr)
                first=header.read_text();self.assertLess(first.index('test_va_list'),first.index('native_declaration'))
                self.assertEqual(build().returncode,0);self.assertEqual(header.read_text(),first)
            else:
                self.assertNotEqual(result.returncode,0)
                self.assertEqual(header.read_text(),native)
                self.assertFalse(header.with_suffix('.h.tmp').exists())
    def test_solaris_tab_guard(self):self.run_case('#define\t_STDIO_H',True)
    def test_space_guard(self):self.run_case('#define _STDIO_H',True)
    def test_missing_guard_fails_atomically(self):self.run_case('#define DIFFERENT',False)
if __name__=='__main__':unittest.main()
