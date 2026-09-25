#!/usr/bin/env python3
"""Pack mods/<Name>/ into build/@<Name>/addons/<name>.pbo (uncompressed) and, if the
signing key ~/.dayz-keys/<SIGN_KEY>.biprivatekey exists (default key: tarbaby), sign it
with tools/sign_pbo.sh and put the public <key>.bikey into build/@<Name>/keys/."""
import hashlib, os, shutil, struct, subprocess, sys

SIGN_KEY = os.environ.get('SIGN_KEY', 'tarbaby')

def build(src, dst_root):
    name = os.path.basename(os.path.normpath(src))
    files = []
    for root, _, names in os.walk(src):
        for n in sorted(names):
            if root == src and (n.startswith('workshop.') or n == 'preview.png'):
                continue  # Workshop metadata, not mod content
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
    # meta.cpp is what the DayZ launcher / DZSA read to tie the folder to its Workshop item
    id_file = os.path.join(src, 'workshop.id')
    if os.path.exists(id_file):
        with open(os.path.join(dst_root, '@' + name, 'meta.cpp'), 'w') as f:
            f.write('protocol = 1;\npublishedid = %s;\nname = "%s";\n' % (open(id_file).read().strip(), name))

    keys = os.path.expanduser('~/.dayz-keys')
    signed = ''
    if os.path.exists(os.path.join(keys, SIGN_KEY + '.biprivatekey')):
        tools = os.path.dirname(os.path.abspath(__file__))
        subprocess.run([os.path.join(tools, 'sign_pbo.sh'), SIGN_KEY, os.path.join(addons, name.lower() + '.pbo')], check=True)
        keydir = os.path.join(dst_root, '@' + name, 'keys')
        os.makedirs(keydir, exist_ok=True)
        shutil.copy(os.path.join(keys, SIGN_KEY + '.bikey'), keydir)
        signed = ', signed with ' + SIGN_KEY
    print('built @%s (%d files%s)' % (name, len(files), signed))

if __name__ == '__main__':
    build(sys.argv[1], sys.argv[2])
