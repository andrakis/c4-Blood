#!/usr/bin/env python3
# protofix.py HEADER SOURCE NAME...: give each NAME's definition in SOURCE the
# parameter list of its prototype in HEADER (MSVC let them differ in const;
# gcc calls that conflicting types). Edits SOURCE in place.
import re, sys
hdr, src, names = sys.argv[1], sys.argv[2], sys.argv[3:]
h = open(hdr, encoding='latin-1').read()
s = open(src, encoding='latin-1').read()

def params(text, name, want_body):
    for m in re.finditer(r'\b' + name + r'\s*\(', text):
        i = m.end() - 1; d = 0; j = i
        while True:
            if text[j] == '(': d += 1
            elif text[j] == ')':
                d -= 1
                if d == 0: break
            j += 1
        after = text[j + 1:j + 40].lstrip()
        if want_body and after.startswith('{'):
            return i, j
        if not want_body and after.startswith(';'):
            return i, j
    return None

for n in names:
    hp = params(h, n, False)
    sp = params(s, n, True)
    if not hp or not sp:
        print('skip', n, bool(hp), bool(sp)); continue
    plist = h[hp[0]:hp[1] + 1]
    s = s[:sp[0]] + plist + s[sp[1] + 1:]
    print('fixed', n, ' '.join(plist.split()))
open(src, 'w', encoding='latin-1').write(s)
