#!/usr/bin/env python3
# verresolve.py EXEDEF QTDEF FILE... : resolve Blood-RE's version conditionals
# (only those) for one build, as unifdef would, and print each FILE's text to
# FILE.out-path given by -o DIR (keeping relative paths). Other conditionals
# (header guards, __cplusplus, ...) are left as they are.
#
#   verresolve.py -o OUTDIR -r ROOT BP121 QT111A files...
#
# EXEDEF is Blood-RE's APPVER_EXEDEF (BS111A, BP121, ...); QTDEF the qtools
# one (QT110, QT111A). A conditional whose expression names only version
# macros is evaluated and dropped; one that mixes them with others is an
# error, reported, so that nothing is guessed.
import os, re, sys

args = sys.argv[1:]
out = root = None
while args and args[0].startswith('-'):
    if args[0] == '-o': out = args[1]; args = args[2:]
    elif args[0] == '-r': root = args[1]; args = args[2:]
    else: sys.exit('bad option ' + args[0])
exedef, qtdef, files = args[0], args[1], args[2:]

BT = {'SHAREWARE': 0, 'SWRETAIL': 1, 'REGISTERED': 2, 'PLASMAPAK': 3}
BL = {'BL110': 199708120, 'BL111': 199708190, 'BL111A': 199709230, 'BL120': 199803180, 'BL121': 199807150}
vals = {}
for k, v in BL.items():
    vals['AV_BR_' + k] = v
    for t, n in (('S', 0), ('SR', 1), ('R', 2), ('P', 3), ('M', None)):
        pass
# the named builds of gamever.h
for ver in ('110', '111', '111A', '120', '121'):
    base = BL['BL' + ver]
    vals['AV_BR_BS' + ver] = base + 0
    vals['AV_BR_BSR' + ver] = base + 1
    vals['AV_BR_BR' + ver] = base + 2
    vals['AV_BR_BP' + ver] = base + 3
    vals['AV_BR_MR' + ver] = base + 2
    vals['AV_BR_MP' + ver] = base + 3
vals['AV_BR_BR120FX'] = vals['AV_BR_BP120FX'] = None  # not built here
for k, v in BT.items():
    vals['AV_BT_' + k] = v
rev = vals['AV_BR_' + exedef]
vals['APPVER_BLOODREV'] = rev
kind = rev % 5
defined = set()
if kind == 0: defined |= {'SHAREWARE'}
if kind == 1: defined |= {'SHAREWARE', 'SWRETAIL'}
if kind == 2: defined |= {'REGISTERED'}
if kind == 3: defined |= {'REGISTERED', 'PLASMAPAK'}
if rev >= vals['AV_BR_BL111A'] or 'PLASMAPAK' in defined:
    defined.add('PLASMAPAK111')
vals['AV_QR_QT110'] = 199708120
vals['AV_QR_QT111A'] = 199709230
vals['APPVER_QTOOLSREV'] = vals['AV_QR_' + qtdef]
vals['MAPEDIT'] = 0
defined.add('MAPEDIT')
KNOWN = set(vals) | {'SHAREWARE', 'SWRETAIL', 'REGISTERED', 'PLASMAPAK', 'PLASMAPAK111', '_3DFX', 'MAPEDIT'}
for k in BT:
    pass

class Unknown(Exception):
    pass

def evaluate(expr):
    e = re.sub(r'/\*.*?\*/', ' ', expr)
    e = re.sub(r'//.*', '', e)
    def dfn(m):
        n = m.group(1) or m.group(2)
        if n not in KNOWN: raise Unknown(n)
        return '1' if (n in defined or (n in vals and vals[n] is not None and n not in BT)) else '0'
    e = re.sub(r'defined\s*\(\s*(\w+)\s*\)|defined\s+(\w+)', dfn, e)
    def ident(m):
        n = m.group(0)
        if n in ('and', 'or', 'not'): return n
        if n not in KNOWN: raise Unknown(n)
        v = vals.get(n)
        if v is None: return '1' if n in defined else '0'
        return str(v)
    e = re.sub(r'[A-Za-z_]\w*', ident, e)
    e = e.replace('&&', ' and ').replace('||', ' or ')
    e = re.sub(r'!(?!=)', ' not ', e)
    return bool(eval(e, {'__builtins__': {}}))

DIR = re.compile(r'^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$')

def process(lines, name):
    outl = []
    # frame: [resolved, taken_any, active]
    stack = []
    def active():
        return all(f[2] for f in stack if f[0])
    for i, line in enumerate(lines):
        m = DIR.match(line)
        if not m:
            if active(): outl.append(line)
            continue
        d, rest = m.group(1), m.group(2).strip()
        if d in ('if', 'ifdef', 'ifndef'):
            try:
                if d == 'if': v = evaluate(rest)
                else:
                    n = rest.split()[0]
                    if n not in KNOWN: raise Unknown(n)
                    v = (n in defined) == (d == 'ifdef')
                stack.append([True, v, v])
            except Unknown:
                stack.append([False, None, True])
                if active(): outl.append(line)
        elif d == 'elif':
            f = stack[-1]
            if not f[0]:
                try:
                    evaluate(rest)
                except Unknown:
                    if active(): outl.append(line)
                    continue
                sys.exit('%s:%d: #elif on a version after a non-version #if' % (name, i + 1))
            if f[1]: f[2] = False
            else:
                try: v = evaluate(rest)
                except Unknown as u: sys.exit('%s:%d: #elif mixes %s' % (name, i + 1, u))
                f[1] = f[2] = v
        elif d == 'else':
            f = stack[-1]
            if f[0]:
                f[2] = not f[1]; f[1] = True
            else:
                stack.pop()
                a = active()
                stack.append(f)
                if a: outl.append(line)
        elif d == 'endif':
            f = stack.pop()
            if not f[0] and active(): outl.append(line)
    if stack: sys.exit('%s: unbalanced conditionals' % name)
    return outl

for fn in files:
    lines = open(fn, encoding='latin-1').read().split('\n')
    res = process(lines, fn)
    dst = os.path.join(out, os.path.relpath(fn, root))
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    open(dst, 'w', encoding='latin-1', newline='').write('\n'.join(res))
print('resolved %d files for %s/%s' % (len(files), exedef, qtdef))
