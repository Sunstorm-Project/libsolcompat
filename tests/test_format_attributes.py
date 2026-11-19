#!/usr/bin/env python3
"""Compile checks; CC may include an explicit target --sysroot."""
import os, shlex, subprocess, tempfile
from pathlib import Path
root = Path(__file__).resolve().parents[1]
cc = shlex.split(os.environ.get('CC', 'cc'))
header = root / 'include/solcompat/snprintf.h'
with tempfile.TemporaryDirectory() as d:
    source = Path(d) / 'check.c'
    def check(body, expect_success):
        source.write_text('#include <stdint.h>\n#include "' + str(header) + '"\n' + body)
        result = subprocess.run(cc + ['-fsyntax-only', '-Werror=format', str(source)], capture_output=True, text=True)
        assert (result.returncode == 0) == expect_success, result.stderr
        if not expect_success:
            assert 'format' in result.stderr and ('expects' in result.stderr or 'specifies' in result.stderr), result.stderr
    check('int main(void){return printf("%ju %zu %td", (uintmax_t)1, (size_t)2, (ptrdiff_t)3);}', True)
    for call in ['printf("%d", "wrong")', 'fprintf(stdout, "%d", "wrong")', 'sprintf(b, "%d", "wrong")', 'snprintf(b, sizeof b, "%d", "wrong")']:
        check('int main(void){char b[16]; return ' + call + ';}', False)
print('format attributes: valid C99 types accepted; four redirected mismatches rejected')
