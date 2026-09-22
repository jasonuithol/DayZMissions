#!/usr/bin/env python3
"""Unpack a PBO: unpbo.py <file.pbo> <out dir>. Handy for reading a mod's scripts and config."""
import os, struct, sys

def cstr(f):
    out = b''
    while True:
        c = f.read(1)
        if c in (b'\0', b''):
            return out.decode('latin1')
        out += c

def unpack(src, out):
    f = open(src, 'rb')
    entries = []
    while True:
        name = cstr(f)
        packing, _, _, _, size = struct.unpack('<IIIII', f.read(20))
        if name == '' and packing == 0x56657273:   # header extension: key/value properties
            while cstr(f) != '':
                cstr(f)
            continue
        if name == '':
            break
        entries.append((name, packing, size))
    skipped = 0
    for name, packing, size in entries:
        data = f.read(size)
        if packing != 0:                            # compressed entries aren't supported
            skipped += 1
            continue
        path = os.path.join(out, name.replace('\\', '/'))
        os.makedirs(os.path.dirname(path) or '.', exist_ok=True)
        with open(path, 'wb') as o:
            o.write(data)
    print('%s: %d files, %d compressed skipped' % (os.path.basename(src), len(entries), skipped))

if __name__ == '__main__':
    unpack(sys.argv[1], sys.argv[2])
