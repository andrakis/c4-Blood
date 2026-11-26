#!/usr/bin/env python3
# port-buildh.py BUILD_H: Blood-RE's build.h onto chocolate-blood's C engine
# (source/build), the changes chocolate-blood's engine.h made and no more.
import re, sys
p = sys.argv[1]
s = open(p).read()
def must(old, new, count=1):
    global s
    if s.count(old) != count:
        sys.exit('expected %d of %r, found %d' % (count, old, s.count(old)))
    s = s.replace(old, new)

# 1. the map structures packed, as the engine reads them from files
lines = s.split('\n')
first = next(i for i, l in enumerate(lines) if re.match(r'struct \w+', l))
lines.insert(first, '#pragma pack(push, 1)  // c4: as the engine (source/build) has them\n')
s = '\n'.join(lines)
must('\nextern "C" {\n', '\n#pragma pack(pop)\n\nextern "C" {\n')

# 2. the clock: an atomic in chocolate-blood's engine, read through a
# function, which is where c4's platforms advance time
must('extern volatile int totalclock;',
     '// c4: chocolate-blood\'s engine keeps the clock behind functions\n'
     'int32_t totalclock_get(void);\n'
     'void totalclock_set(int32_t value);\n'
     'void totalclock_add(int32_t value);\n'
     'void totalclock_sub(int32_t value);\n'
     '#define totalclock totalclock_get()')

# 3. the engine functions Blood replaces (replace.cpp) are pointers there
for old, new in (
    ('void uninitengine(void);', 'extern void (*uninitengine)(void);'),
    ('int getpalookup(int, int);', 'extern int (*getpalookup)(int, int);'),
    ('void initspritelists(void);', 'extern void (*initspritelists)(void);'),
    ('int insertsprite(short, short);', 'extern int (*insertsprite)(short, short);'),
    ('void deletesprite(short);', 'extern int (*deletesprite)(short);'),
    ('int changespritesect(short, short);', 'extern int (*changespritesect)(short, short);'),
    ('int changespritestat(short, short);', 'extern int (*changespritestat)(short, short);'),
    ('int animateoffs(short a1, ushort a2);', 'extern int (*animateoffs)(short a1, ushort a2);'),
    ('void fixtransluscence(void);', 'void fixtransluscence(intptr_t);'),
):
    must(old, new)

# 4. Watcom's register conventions for a.asm: the engine's are C now
s = re.sub(r'\n#pragma aux [^\n]*', '', s)
open(p, 'w').write(s)
print('ok')
