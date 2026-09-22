#!/usr/bin/env python3
"""Decode a binarized config.bin to readable config text: derap.py <config.bin>"""
import struct, sys

d = open(sys.argv[1], 'rb').read()
assert d[:4] == b'\0raP', 'not a binarized config'

def cstr(o):
    e = d.index(b'\0', o)
    return d[o:e].decode('latin1'), e + 1

def cint(o):
    v = s = 0
    while True:
        b = d[o]; o += 1
        v |= (b & 0x7f) << s; s += 7
        if b < 0x80:
            return v, o

def value(t, o):
    if t in (0, 4):
        s, o = cstr(o)
        return ('"%s"' % s if t == 0 else s), o
    if t == 1:
        return repr(round(struct.unpack_from('<f', d, o)[0], 6)), o + 4
    if t == 2:
        return str(struct.unpack_from('<i', d, o)[0]), o + 4
    if t == 3:
        return array(o)
    raise ValueError('unknown value type %d' % t)

def array(o):
    n, o = cint(o)
    out = []
    for _ in range(n):
        v, o = value(d[o], o + 1)
        out.append(v)
    return '{' + ', '.join(out) + '}', o

def body(o, ind, res):
    parent, o = cstr(o)
    n, o = cint(o)
    tab = '\t' * ind
    for _ in range(n):
        t = d[o]; o += 1
        if t == 0:
            name, o = cstr(o)
            off = struct.unpack_from('<I', d, o)[0]; o += 4
            sub = []
            p = body(off, ind + 1, sub)
            res.append('%sclass %s%s\n%s{' % (tab, name, ': ' + p if p else '', tab))
            res.extend(sub)
            res.append(tab + '};')
        elif t == 1:
            st = d[o]
            name, o = cstr(o + 1)
            v, o = value(st, o)
            res.append('%s%s = %s;' % (tab, name, v))
        elif t == 2:
            name, o = cstr(o)
            v, o = array(o)
            res.append('%s%s[] = %s;' % (tab, name, v))
        elif t == 3:
            name, o = cstr(o)
            res.append('%sclass %s;' % (tab, name))
        elif t == 4:
            name, o = cstr(o)
            res.append('%sdelete %s;' % (tab, name))
        elif t == 5:
            o += 4
            name, o = cstr(o)
            v, o = array(o)
            res.append('%s%s[] += %s;' % (tab, name, v))
    return parent

res = []
body(16, 0, res)
print('\n'.join(res))
