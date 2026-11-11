from pathlib import Path
import importlib.util
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('native_headers',ROOT/'tools/materialize-native-headers.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)

class NativeHeaders(unittest.TestCase):
    def test_forwarding_order_existing_vendor_and_c_overlays(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);source=root/'source';prep=source/'include/sysroot-prep';prep.mkdir(parents=True)
            overlay=source/'sysroot-overlay/usr/include';overlay.mkdir(parents=True)
            (prep/'stdio.h.prepend').write_text('#define BEFORE_VENDOR 17\n')
            (prep/'stdio.h.append').write_text('#define AFTER_VENDOR 23\n')
            (prep/'existing.h.append').write_text('#define LATER_REFRESH 1\n')
            (prep/'usr/openwin/include/X11').mkdir(parents=True)
            (prep/'usr/openwin/include/X11/Xfuncproto.h.append').write_text('separate root\n')
            (overlay/'mntent.h').write_text('#define AUTHORED_MNTENT 1\n')
            (overlay/'cstdlib').write_text('C++ overlay handled separately\n')
            dest=root/'fixed';dest.mkdir();(dest/'existing.h').write_text('vendor fixed content\n')
            vendor=root/'vendor';vendor.mkdir();(vendor/'stdio.h').write_text('#if BEFORE_VENDOR != 17\n#error prepend ordering\n#endif\n#define VENDOR_PRESENT 31\n')
            m.materialize(source,dest)
            self.assertEqual((dest/'existing.h').read_text(),'vendor fixed content\n')
            self.assertEqual((dest/'mntent.h').read_text(),'#define AUTHORED_MNTENT 1\n')
            self.assertFalse((dest/'cstdlib').exists());self.assertFalse((dest/'usr').exists())
            caller=root/'check.c';caller.write_text('#include <stdio.h>\n#if AFTER_VENDOR != 23 || VENDOR_PRESENT != 31\n#error missing forwarding API\n#endif\n')
            subprocess.run(['cc','-fsyntax-only','-nostdinc','-I'+str(dest),'-I'+str(vendor),str(caller)],check=True)
            (prep/'stdio.h.append').write_text('#define AFTER_VENDOR 29\n')
            m.materialize(source,dest)
            self.assertIn('AFTER_VENDOR 29',(dest/'stdio.h').read_text())
            self.assertNotIn('AFTER_VENDOR 23',(dest/'stdio.h').read_text())
    def test_abi_types_requires_existing_fixed_header(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);prep=root/'include/sysroot-prep/sys';prep.mkdir(parents=True)
            (root/'sysroot-overlay/usr/include').mkdir(parents=True)
            (prep/'types.h.append').write_text('append\n')
            dest=root/'fixed'
            with self.assertRaisesRegex(ValueError,'ABI-patched'):m.materialize(root,dest)
            (dest/'sys').mkdir(parents=True);(dest/'sys/types.h').write_text('ABI fixed header\n')
            m.materialize(root,dest)
            self.assertEqual((dest/'sys/types.h').read_text(),'ABI fixed header\n')
    def test_symlink_parent_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);prep=root/'include/sysroot-prep/sys';prep.mkdir(parents=True)
            (root/'sysroot-overlay/usr/include').mkdir(parents=True)
            (prep/'socket.h.append').write_text('append\n')
            dest=root/'fixed';dest.mkdir();victim=root/'vendor';victim.mkdir()
            (dest/'sys').symlink_to(victim,target_is_directory=True)
            with self.assertRaisesRegex(ValueError,'symlink'):m.materialize(root,dest)
            self.assertFalse((victim/'socket.h').exists())
    def test_symlink_destination_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);(root/'include/sysroot-prep').mkdir(parents=True)
            (root/'sysroot-overlay/usr/include').mkdir(parents=True)
            (root/'include/sysroot-prep/test.h.append').write_text('append\n')
            dest=root/'fixed';dest.mkdir();victim=root/'victim';victim.write_text('unchanged')
            (dest/'test.h').symlink_to(victim)
            with self.assertRaisesRegex(ValueError,'symlink'):m.materialize(root,dest)
            self.assertEqual(victim.read_text(),'unchanged')
if __name__=='__main__':unittest.main()
