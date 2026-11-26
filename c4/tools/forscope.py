#!/usr/bin/env python3
# forscope.py FILE...: Watcom C++ 10.6 kept a for statement's declared
# variable alive to the end of the enclosing block (the old, pre-standard
# rule), and Blood-RE uses it after the loop. Where a name declared in a
# for's init is used after the loop in the enclosing block, the declaration
# moves before the for: "int i; for (i = 0; ...)". Under the old rule that
# means the same; under ISO C++ it is what the old rule meant. A for that is
# itself the body of an if/else/while is reported, not changed.
import re, sys

def blank(s):
    """Comments and literals to spaces (same length), for analysis."""
    out = list(s); i = 0; n = len(s)
    while i < n:
        if s.startswith('//', i):
            j = s.find('\n', i); j = n if j < 0 else j
            for k in range(i, j): out[k] = ' '
            i = j
        elif s.startswith('/*', i):
            j = s.find('*/', i + 2); j = n if j < 0 else j + 2
            for k in range(i, j):
                if s[k] != '\n': out[k] = ' '
            i = j
        elif s[i] in '"\'':
            q = s[i]; j = i + 1
            while j < n and s[j] != q:
                if s[j] == '\\': j += 1
                j += 1
            for k in range(i + 1, min(j, n)): out[k] = ' '
            i = j + 1
        elif s[i] == '#' and (i == 0 or s[i - 1] == '\n'):
            j = s.find('\n', i)
            while j > 0 and s[j - 1] == '\\': j = s.find('\n', j + 1)
            j = n if j < 0 else j
            for k in range(i, j): out[k] = ' '
            i = j
        else:
            i += 1
    return ''.join(out)

def match(t, i):
    """index of the bracket closing the one at i"""
    o = t[i]; c = {'(': ')', '{': '}', '[': ']'}[o]; d = 0
    for j in range(i, len(t)):
        if t[j] == o: d += 1
        elif t[j] == c:
            d -= 1
            if d == 0: return j
    raise ValueError('unbalanced at %d' % i)

def skipws(t, i):
    while i < len(t) and t[i] in ' \t\r\n': i += 1
    return i

def stmt_end(t, i):
    """index just past the statement starting at i"""
    i = skipws(t, i)
    if t[i] == '{': return match(t, i) + 1
    m = re.match(r'(if|for|while|switch)\b\s*', t[i:])
    if m:
        p = skipws(t, i + m.end())
        e = stmt_end(t, match(t, p) + 1)
        if m.group(1) == 'if':
            k = skipws(t, e)
            if re.match(r'else\b', t[k:]):
                e = stmt_end(t, k + 4)
        return e
    m = re.match(r'do\b', t[i:])
    if m:
        e = stmt_end(t, i + 2)
        k = skipws(t, e)
        k = match(t, skipws(t, k + 5))
        return t.index(';', k) + 1
    m = re.match(r'else\b', t[i:])
    if m: return stmt_end(t, i + 4)
    d = 0; j = i
    while j < len(t):
        c = t[j]
        if c in '([{': d += 1
        elif c in ')]}': d -= 1
        elif c == ';' and d == 0: return j + 1
        j += 1
    raise ValueError('no statement end at %d' % i)

def enclosing_block_end(t, i):
    d = 0; j = i - 1
    while j >= 0:
        if t[j] == '}': d += 1
        elif t[j] == '{':
            if d == 0: return match(t, j)
            d -= 1
        j -= 1
    return len(t)

DECL = re.compile(r'\s*((?:const\s+|unsigned\s+|signed\s+)*(?:int|short|long|char|byte|uchar|ushort|uint|ulong|BOOL|[A-Z][A-Za-z0-9_]*))\s+(\**\s*\w+\s*(?:=[^,;]*)?(?:,\s*\**\s*\w+\s*(?:=[^,;]*)?)*)\s*$')

def process(path):
    s = open(path, encoding='latin-1').read()
    edits = []; report = []
    t = blank(s)
    for m in re.finditer(r'\bfor\s*\(', t):
        p = m.end() - 1
        close = match(t, p)
        semi = t.find(';', p)
        if semi < 0 or semi > close: continue
        init = t[p + 1:semi]
        dm = DECL.match(init)
        if not dm: continue
        typ = dm.group(1)
        decls = [x.strip() for x in re.split(r',(?![^(]*\))', s[p + 1 + dm.start(2):p + 1 + dm.end(2)])]
        names = [re.match(r'\**\s*(\w+)', x).group(1) for x in decls]
        end = stmt_end(t, m.start())
        bend = enclosing_block_end(t, m.start())
        after = t[end:bend]
        used = [n for n in names if re.search(r'\b%s\b' % n, after)]
        if not used: continue
        k = m.start() - 1
        while k >= 0 and t[k] in ' \t\r\n': k -= 1
        prev = t[k] if k >= 0 else ';'
        line = s.count('\n', 0, m.start()) + 1
        if prev not in ';{}:':
            report.append('%s:%d: for as a sub-statement, %s used after it: not changed' % (path, line, ','.join(used)))
            continue
        # "int i = 0, *p = q" -> "int i, *p;" before, "i = 0, p = q" in the for
        stars = [re.match(r'(\**)', x).group(1) for x in decls]
        hoist = '%s %s; ' % (s[p + 1 + dm.start(1):p + 1 + dm.end(1)], ', '.join(st + n for st, n in zip(stars, names)))
        inits = [re.sub(r'^\**\s*', '', x) for x in decls if '=' in x]
        new_init = ', '.join(inits)
        edits.append((m.start(), m.start(), hoist))
        edits.append((p + 1, semi, new_init))
        report.append('%s:%d: hoisted %s' % (path, line, ', '.join(names)))
    for a, b, txt in sorted(edits, reverse=True):
        s = s[:a] + txt + s[b:]
    if edits:
        open(path, 'w', encoding='latin-1', newline='').write(s)
    return report

for f in sys.argv[1:]:
    for r in process(f):
        print(r)
