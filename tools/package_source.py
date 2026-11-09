#!/usr/bin/env python3
"""Package manifest entries only, without private Git history or build outputs."""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import re
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path, help='ZIP path outside the source directory')
    args = parser.parse_args()
    output = args.output.resolve()
    if output.is_relative_to(ROOT):
        parser.error('Choose an output path outside the source directory')
    subprocess.run([sys.executable, '-X', 'utf8', str(ROOT/'tools/release_audit.py')], check=True)
    entries = []
    for line in (ROOT/'SOURCE_SHA256SUMS').read_text(encoding='utf-8').splitlines():
        if not line or line.startswith('#'):
            continue
        match = re.fullmatch(r'([0-9a-f]{64})  (.+)', line)
        if not match:
            raise ValueError('Malformed source manifest')
        name = match.group(2)
        path = PurePosixPath(name)
        if path.is_absolute() or any(p in ('..', '.git') for p in path.parts):
            raise ValueError('Unsafe source entry')
        data = (ROOT/name).read_bytes()
        if hashlib.sha256(data).hexdigest() != match.group(1):
            raise ValueError('Changed file: '+name)
        entries.append((name, data))
    entries.append(('SOURCE_SHA256SUMS', (ROOT/'SOURCE_SHA256SUMS').read_bytes()))
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(entries):
            item = zipfile.ZipInfo('USB-DB25-Marker-Box/'+name, (2026, 11, 9, 0, 0, 0))
            item.compress_type = zipfile.ZIP_DEFLATED
            item.external_attr = 0o100644 << 16
            archive.writestr(item, data)
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    output.with_suffix(output.suffix+'.sha256').write_text(digest+'  '+output.name+'\n', encoding='utf-8')
    print('Packaged', len(entries), 'files:', output.name)
    print('SHA-256:', digest)
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
