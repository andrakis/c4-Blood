#!/usr/bin/env python3
# port-misc4.py ROOT: the last link-time ports, applied once.
import os, re, sys
R = sys.argv[1]
def edit(fn, pairs):
    p = os.path.join(R, fn)
    s = open(p, encoding='latin-1').read()
    for old, new in pairs:
        if s.count(old) != 1:
            sys.exit('%s: expected one %r, found %d' % (fn, old, s.count(old)))
        s = s.replace(old, new)
    open(p, 'w', encoding='latin-1', newline='').write(s)

# fire.cpp: the fire tile from the fire buffer, turned a quarter and through
# the colour table: Watcom inline assembly, now C (chocolate-blood's)
p = os.path.join(R, 'source/blood/fire.cpp')
s = open(p, encoding='latin-1').read()
m = re.search(r'void DoFireFramePragma\(byte \*, byte \*\);\n\n#pragma aux DoFireFramePragma = \\\n(?:"[^\n]*"\\\n)+parm \[ebx\] \[edi\]\\\nmodify \[eax ecx edx esi edi\]\n', s)
if not m: sys.exit('fire.cpp: the pragma is not as expected')
s = s[:m.start()] + '''// c4: Watcom inline assembly; the same in C (chocolate-blood's)
void DoFireFramePragma(byte *a1, byte *a2)
{
    byte* s = FrameBuffer;
    int i = fireSize;
    do
    {
        int j = fireSize;
        byte* p = a2;
        do
        {
            *p = a1[*s++];
            p += fireSize;
        } while (--j);
        a2++;
    } while (--i);
}
''' + s[m.end():]
open(p, 'w', encoding='latin-1', newline='').write(s)

# usrhooks.cpp: audiolib (C) calls these, so C linkage, with its header's
# types
edit('source/blood/usrhooks.cpp', [
    ('#include "USRHOOKS.H"', 'extern "C" {  // c4: audiolib, which calls these, is C\n#include "USRHOOKS.H"\n}'),
    ('int USRHOOKS_GetMem(void **ptr, unsigned long size)', 'int USRHOOKS_GetMem(void **ptr, uint32_t size)'),
])
print('ok')
