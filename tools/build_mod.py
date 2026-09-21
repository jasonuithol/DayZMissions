#!/usr/bin/env python3
"""Pack mods/<Name>/ into build/@<Name>/addons/<name>.pbo (unsigned, uncompressed)."""
import hashlib, os, struct, sys

def build(src, dst_root):
    name = os.path.basename(os.path.normpath(src))
    files = []
    for root, _, names in os.walk(src):
        for n in sorted(names):
            full = os.path.join(root, n)
            files.append((os.path.relpath(full, src).replace('/', '\\'), open(full, 'rb').read()))
    files.sort()

    entry = lambda fname, packing, size: fname.encode() + b'\0' + struct.pack('<IIIII', packing, size, 0, 0, size)
    out = entry('', 0x56657273, 0) + b'prefix\0' + name.encode() + b'\0\0'
    for fname, data in files:
        out += entry(fname, 0, len(data))
    out += entry('', 0, 0)
    out += b''.join(data for _, data in files)
    out += b'\0' + hashlib.sha1(out).digest()

    addons = os.path.join(dst_root, '@' + name, 'addons')
    os.makedirs(addons, exist_ok=True)
    with open(os.path.join(addons, name.lower() + '.pbo'), 'wb') as f:
        f.write(out)
    with open(os.path.join(dst_root, '@' + name, 'mod.cpp'), 'w') as f:
        f.write('name = "%s";\n' % name)
    print('built @%s (%d files)' % (name, len(files)))

if __name__ == '__main__':
    build(sys.argv[1], sys.argv[2])
