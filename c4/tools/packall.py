#!/usr/bin/env python3
# packall.py FILE...: Blood was built by Watcom with 1-byte structure packing
# (Blood-RE names fields by offset: PLAYER's "int at1" is at offset 1), and
# its structures are read and written as bytes (demos, saves, maps, QAV,
# SEQ). Each of Blood's own sources gets "#pragma pack(push, 1)" after its
# #includes and "#pragma pack(pop)" at its end (a header: before its guard's
# #endif), so that what it defines is packed as Watcom packed it and what it
# includes (the C library, MACT, audiolib) is not.
import re, sys
PUSH = '#pragma pack(push, 1)  // c4: Watcom\'s packing, as Blood was built (packall.py)'
POP = '#pragma pack(pop)  // c4: packall.py'
for path in sys.argv[1:]:
    s = open(path, encoding='latin-1').read()
    if 'packall.py' in s:
        continue
    lines = s.split('\n')
    depth = 0; last_inc = None; depth_at = []
    for i, l in enumerate(lines):
        m = re.match(r'\s*#\s*(if|ifdef|ifndef|endif|include)\b', l)
        if not m:
            depth_at.append(depth); continue
        d = m.group(1)
        if d in ('if', 'ifdef', 'ifndef'): depth += 1
        elif d == 'endif': depth -= 1
        depth_at.append(depth)
        if d == 'include': last_inc = i
    header = path.endswith('.h')
    guard = 1 if header and re.search(r'^\s*#\s*ifndef\s+\w+\s*\n\s*#\s*define', s, re.M) else 0
    if last_inc is None:
        # no includes: after the guard's #define, or at the top
        at = 0
        if guard:
            at = next(i for i, l in enumerate(lines) if re.match(r'\s*#\s*define', l)) + 1
    else:
        at = last_inc + 1
        # out of any conditional the include is in (beyond the guard)
        while depth_at[at - 1] > guard:
            m = re.match(r'\s*#\s*endif\b', lines[at])
            at += 1
            if m and depth_at[at - 1] <= guard:
                break
    lines.insert(at, PUSH)
    if guard:
        k = max(i for i, l in enumerate(lines) if re.match(r'\s*#\s*endif\b', l))
        lines.insert(k, POP)
    else:
        while lines and lines[-1] == '':
            lines.pop()
        lines += [POP, '']
    open(path, 'w', encoding='latin-1', newline='').write('\n'.join(lines))
    print(path, 'at line', at + 1)
