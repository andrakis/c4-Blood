#!/usr/bin/env python3
# incslash.py DIR: backslashes to slashes in #include lines under DIR.
import os, re, sys
n = 0
for d, _, fs in os.walk(sys.argv[1]):
    for f in fs:
        if not re.search(r'\.(c|cpp|h)$', f, re.I): continue
        p = os.path.join(d, f)
        s = open(p, encoding='latin-1').read()
        s2 = re.sub(r'^(\s*#\s*include\s*[<"])([^>"]*)', lambda m: m.group(1) + m.group(2).replace('\\', '/'), s, flags=re.M)
        if s2 != s:
            open(p, 'w', encoding='latin-1', newline='').write(s2)
            n += 1
print('files', n)
