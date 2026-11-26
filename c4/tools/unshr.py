#!/usr/bin/env python3
# unshr.py SHARE.SHR OUTDIR [NAME...]: extract Blood's shareware installer
# archive. Each entry: an 8.3 name (NUL-padded), its compressed size as a
# 32-bit word 136 bytes after the name, and at 168 bytes after the name the
# data, PKWARE DCL "implode" (header 00 06), which this explodes (a port of
# zlib's contrib/blast.c). Run with python3 -I; writes only into OUTDIR.
import os, re, struct, sys

LITLEN = [11, 124, 8, 7, 28, 7, 188, 13, 76, 4, 10, 8, 12, 10, 12, 10, 8, 23, 8,
          9, 7, 6, 7, 8, 7, 6, 55, 8, 23, 24, 12, 11, 7, 9, 11, 12, 6, 7, 22, 5,
          7, 24, 6, 11, 9, 6, 7, 22, 7, 11, 38, 7, 9, 8, 25, 11, 8, 11, 9, 12,
          8, 12, 5, 38, 5, 38, 5, 11, 7, 5, 6, 21, 6, 10, 53, 8, 7, 24, 10, 27,
          44, 253, 253, 253, 252, 252, 252, 13, 12, 45, 12, 45, 12, 61, 12, 45,
          44, 173]
LENLEN = [2, 35, 36, 53, 38, 23]
DISTLEN = [2, 20, 53, 230, 247, 151, 248]
BASE = [3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264]
EXTRA = [0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8]
MAXBITS = 13

def construct(rep):
    length = []
    for b in rep:
        length += [b & 15] * ((b >> 4) + 1)
    count = [0] * (MAXBITS + 1)
    for l in length:
        count[l] += 1
    offs = [0] * (MAXBITS + 1)
    for l in range(1, MAXBITS):
        offs[l + 1] = offs[l] + count[l]
    symbol = [0] * len(length)
    for s, l in enumerate(length):
        if l:
            symbol[offs[l]] = s
            offs[l] += 1
    return count, symbol

LITCODE, LENCODE, DISTCODE = construct(LITLEN), construct(LENLEN), construct(DISTLEN)

class Bits:
    def __init__(self, data):
        self.d = data; self.p = 0; self.buf = 0; self.cnt = 0
    def bits(self, need):
        while self.cnt < need:
            if self.p >= len(self.d):
                raise EOFError('ran out of input')
            self.buf |= self.d[self.p] << self.cnt
            self.p += 1; self.cnt += 8
        v = self.buf & ((1 << need) - 1)
        self.buf >>= need; self.cnt -= need
        return v
    def decode(self, h):
        count, symbol = h
        code = first = index = 0
        for l in range(1, MAXBITS + 1):
            code |= self.bits(1) ^ 1
            c = count[l]
            if code < first + c:
                return symbol[index + code - first]
            index += c; first += c
            first <<= 1; code <<= 1
        raise ValueError('bad code')

def explode(data):
    s = Bits(data)
    lit = s.bits(8); dictb = s.bits(8)
    if lit > 1 or dictb < 4 or dictb > 6:
        raise ValueError('not DCL implode (%d %d)' % (lit, dictb))
    out = bytearray()
    while True:
        if s.bits(1):
            sym = s.decode(LENCODE)
            ln = BASE[sym] + s.bits(EXTRA[sym])
            if ln == 519:
                break
            sym = 2 if ln == 2 else dictb
            dist = (s.decode(DISTCODE) << sym) + s.bits(sym) + 1
            if dist > len(out):
                raise ValueError('distance too far back')
            for _ in range(ln):
                out.append(out[-dist])
        else:
            out.append(s.decode(LITCODE) if lit else s.bits(8))
    return bytes(out)

def entries(d):
    pos = 0
    first = re.search(rb'[A-Z0-9_~\-]{1,8}\.[A-Z0-9]{1,3}\0', d)
    n = first.start()
    while n + 168 <= len(d):
        name = d[n:n + 13].split(b'\0')[0].decode('ascii')
        size = struct.unpack_from('<I', d, n + 136)[0]
        yield name, d[n + 168:n + 168 + size]
        n += 168 + size

if __name__ == '__main__':
    src, out = sys.argv[1], sys.argv[2]
    want = set(a.upper() for a in sys.argv[3:])
    os.makedirs(out, exist_ok=True)
    d = open(src, 'rb').read()
    for name, comp in entries(d):
        if not re.fullmatch(r'[A-Z0-9_~\-]{1,8}(\.[A-Z0-9]{1,3})?', name):
            raise SystemExit('bad entry name %r' % name)
        if want and name not in want:
            continue
        data = explode(comp)
        open(os.path.join(out, name), 'wb').write(data)
        print('%-13s %9d -> %9d' % (name, len(comp), len(data)))
