#!/usr/bin/env python3
# includefix.py SOURCE_ROOT: spell every project #include as its file is
# spelled, for case-sensitive file systems. A name is looked up as the build
# looks it up: beside the includer, then in the include directories, and
# rewritten only when the exact spelling is missing and one file matches
# without case. Edits the sources in place; prints each change.
import os, re, sys
root = sys.argv[1]
INCDIRS = ['blood', 'blood/qtools/include', 'build', 'mact', 'audiolib']
BUILT = ['blood', 'build', 'mact', 'audiolib']
listing = {}
def names(d):
    if d not in listing:
        listing[d] = os.listdir(d) if os.path.isdir(d) else []
    return listing[d]
inc = re.compile(r'^(\s*#\s*include\s*)([<"])([^>"]+)([>"])', re.M)
changes = 0
for top in BUILT:
    for d, _, fs in os.walk(os.path.join(root, top)):
        for f in fs:
            if not re.search(r'\.(c|cpp|h)$', f, re.I):
                continue
            p = os.path.join(d, f)
            s = open(p, encoding='latin-1').read()
            def fix(m):
                global changes
                name = m.group(3)
                if '/' in name:
                    return m.group(0)
                search = ([d] if m.group(2) == '"' else []) + [os.path.join(root, x) for x in INCDIRS]
                for sd in search:
                    if name in names(sd):
                        return m.group(0)
                for sd in search:
                    hits = [n for n in names(sd) if n.lower() == name.lower()]
                    if len(hits) == 1:
                        changes += 1
                        print('%s: %s -> %s' % (os.path.relpath(p, root), name, hits[0]))
                        return m.group(1) + m.group(2) + hits[0] + m.group(4)
                return m.group(0)
            s2 = inc.sub(fix, s)
            if s2 != s:
                open(p, 'w', encoding='latin-1', newline='').write(s2)
print('changed', changes)
