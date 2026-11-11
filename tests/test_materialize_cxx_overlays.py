import pathlib,tempfile,subprocess,sys,unittest
helper=pathlib.Path(__file__).resolve().parents[1]/'tools/materialize-cxx-overlays.py'
class Tests(unittest.TestCase):
 def fixture(self,d):
  root=pathlib.Path(d);src=root/'source';dst=root/'destination';src.mkdir();dst.mkdir()
  for name in ('cmath','cstdlib'):
   (src/name).write_text('#define _SOLCOMPAT_OVERRIDE_'+name.upper()+'\n#include_next <'+name+'>\n')
   (dst/name).write_text('original '+name+'\n')
  return src,dst
 def runhelper(self,s,d):return subprocess.run([sys.executable,str(helper),str(s),str(d)],capture_output=True)
 def test_preserved_idempotent_refresh(self):
  with tempfile.TemporaryDirectory() as d:
   s,t=self.fixture(d);self.assertEqual(self.runhelper(s,t).returncode,0)
   first={p.relative_to(t):p.read_bytes() for p in t.rglob('*') if p.is_file()}
   self.assertEqual(self.runhelper(s,t).returncode,0)
   self.assertEqual(first,{p.relative_to(t):p.read_bytes() for p in t.rglob('*') if p.is_file()})
   self.assertEqual((t/'.solcompat-upstream/cmath').read_text(),'original cmath\n')
 def test_existing_fixed_temp_symlink_is_not_followed(self):
  with tempfile.TemporaryDirectory() as d:
   s,t=self.fixture(d);victim=pathlib.Path(d)/'victim';victim.write_text('unchanged')
   (t/'cmath.solcompat-new').symlink_to(victim)
   self.assertEqual(self.runhelper(s,t).returncode,0)
   self.assertEqual(victim.read_text(),'unchanged')
 def test_irrecoverable_old_wrapper_refused_before_mutation(self):
  with tempfile.TemporaryDirectory() as d:
   s,t=self.fixture(d);(t/'cstdlib').write_text('_SOLCOMPAT_OVERRIDE_CSTDLIB\n');before=(t/'cmath').read_bytes()
   self.assertNotEqual(self.runhelper(s,t).returncode,0);self.assertEqual((t/'cmath').read_bytes(),before);self.assertFalse((t/'.solcompat-upstream').exists())
 def test_malformed_source_refused_before_mutation(self):
  with tempfile.TemporaryDirectory() as d:
   s,t=self.fixture(d);(s/'cstdlib').write_text('broken');before=(t/'cmath').read_bytes()
   self.assertNotEqual(self.runhelper(s,t).returncode,0);self.assertEqual((t/'cmath').read_bytes(),before);self.assertFalse((t/'.solcompat-upstream').exists())
if __name__=='__main__':unittest.main()
