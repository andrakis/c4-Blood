#!/usr/bin/env python3
# port-clock.py SRCDIR: gGameClock and the engine's totalclock behind
# functions, as chocolate-blood has them. A timer interrupt moved them under
# DOS; here a read is where the platform advances time and runs the timer
# (c4blood_clock_read), so that loops like credits.cpp's
# "while (gGameClock < gFrameClock) { }" end on one thread.
import os, sys
d = sys.argv[1]
def edit(fn, pairs):
    p = os.path.join(d, fn)
    s = open(p, encoding='latin-1').read()
    for old, new in pairs:
        if s.count(old) != 1:
            sys.exit('%s: expected one %r, found %d' % (fn, old, s.count(old)))
        s = s.replace(old, new)
    open(p, 'w', encoding='latin-1', newline='').write(s)

edit('globals.h', [
    ('extern long volatile gGameClock;',
     '// c4: the clock behind functions; reading it advances the platform\'s time\n'
     'int32_t gGameClock_get(void);\n'
     'void gGameClock_set(int32_t value);\n'
     'void gGameClock_add(int32_t value);\n'
     'void gGameClock_sub(int32_t value);\n'
     '#define gGameClock gGameClock_get()'),
])
edit('globals.cpp', [
    ('long volatile gGameClock;',
     '// c4: Blood\'s clock, behind functions (globals.h)\n'
     'extern "C" void c4blood_clock_read(void);\n'
     'static int32_t gGameClock_val;\n'
     'int32_t gGameClock_get(void) { c4blood_clock_read(); return gGameClock_val; }\n'
     'void gGameClock_set(int32_t value) { gGameClock_val = value; }\n'
     'void gGameClock_add(int32_t value) { gGameClock_val += value; }\n'
     'void gGameClock_sub(int32_t value) { gGameClock_val -= value; }'),
    ('    ++gGameClock;\n    totalclock = gGameClock;',
     '    // c4: the timer interrupt; not through the read, which runs the timer\n'
     '    gGameClock_val++;\n    totalclock_set(gGameClock_val);'),
])
edit('blood.cpp', [('    gGameClock = 0;', '    gGameClock_set(0);')])
edit('cdrom.cpp', [('    gGameClock = oc;', '    gGameClock_set(oc);')])
edit('credits.cpp', [
    ('    gGameClock = 0;', '    gGameClock_set(0);'),
])
p = os.path.join(d, 'credits.cpp')
s = open(p, encoding='latin-1').read()
assert s.count('    gGameClock = gFrameClock = 0;') == 2
s = s.replace('    gGameClock = gFrameClock = 0;', '    gGameClock_set(0); gFrameClock = 0;')
open(p, 'w', encoding='latin-1', newline='').write(s)
edit('loadsave.cpp', [
    ('    gGameClock = 0;', '    gGameClock_set(0);'),
    ('    gGameClock = nGameClock;', '    gGameClock_set(nGameClock);'),
])
edit('network.cpp', [
    ('    gNetFifoClock = gFrameClock = gGameClock = 0;', '    gGameClock_set(0); gNetFifoClock = gFrameClock = 0;'),
])
p = os.path.join(d, 'network.cpp')
s = open(p, encoding='latin-1').read()
assert s.count('gGameClock -= t<<2;') == 2
s = s.replace('gGameClock -= t<<2;', 'gGameClock_sub(t<<2);')
open(p, 'w', encoding='latin-1', newline='').write(s)
print('ok')
