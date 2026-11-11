#!/usr/bin/env python3
"""Preserve upstream C++ headers before applying authored compatibility wrappers."""
import argparse,os,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('source');p.add_argument('destination');a=p.parse_args()
src=Path(a.source);dst=Path(a.destination);keep=dst/'.solcompat-upstream'
def atomic_write(path,content):
    fd,name=tempfile.mkstemp(prefix=path.name+'.solcompat-',dir=path.parent)
    temporary=Path(name)
    try:
        with os.fdopen(fd,'wb') as stream:stream.write(content)
        temporary.chmod(0o644);os.replace(temporary,path)
    finally:
        if temporary.exists():temporary.unlink()
for name in ('cmath','cstdlib'):
    target=dst/name; original=keep/name
    if target.is_symlink() or keep.is_symlink() or original.is_symlink():raise SystemExit('refusing symlink C++ header path')
    if not target.is_file():raise SystemExit('missing upstream C++ header: '+name)
    text=target.read_text()
    wrapped='_SOLCOMPAT_OVERRIDE_' in text
    if wrapped and not original.is_file():raise SystemExit('original upstream header was overwritten; fresh compiler install required: '+name)
    if not wrapped and original.exists() and original.read_bytes()!=target.read_bytes():raise SystemExit('upstream header changed; explicit fresh install required: '+name)
    if original.exists() and '_SOLCOMPAT_OVERRIDE_' in original.read_text():raise SystemExit('retained header is a wrapper')
for name in ('cmath','cstdlib'):
    text=(src/name).read_text();needle='\n#include_next <'+name+'>\n'
    if text.count(needle)!=1:raise SystemExit('unexpected wrapper include contract: '+name)
for name in ('cmath','cstdlib'):
    target=dst/name;original=keep/name
    keep.mkdir(exist_ok=True)
    if not original.exists():atomic_write(original,target.read_bytes())
    text=(src/name).read_text();needle='\n#include_next <'+name+'>\n'
    if text.count(needle)!=1:raise SystemExit('unexpected wrapper include contract')
    text=text.replace(needle,'\n#include ".solcompat-upstream/'+name+'"\n')
    atomic_write(target,text.encode())
