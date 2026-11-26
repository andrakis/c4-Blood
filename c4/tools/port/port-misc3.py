#!/usr/bin/env python3
# port-misc3.py ROOT: more of the port, applied once.
import os, sys
R = sys.argv[1]
def edit(fn, pairs):
    p = os.path.join(R, fn)
    s = open(p, encoding='latin-1').read()
    for old, new in pairs:
        if s.count(old) != 1:
            sys.exit('%s: expected one %r, found %d' % (fn, old, s.count(old)))
        s = s.replace(old, new)
    open(p, 'w', encoding='latin-1', newline='').write(s)

# the platform's entry points (build/system.h; build/sound.h shares its name
# with Blood's sound.h, so Sound_Init is declared here)
edit('source/blood/blood.cpp', [
    ('void replace_hook();\n\nint main(int argc, char **argv)',
     '#include "system.h"\nextern "C" void Sound_Init(int rate);\nvoid replace_hook();\n\nint main(int argc, char **argv)'),
])
# chocolate-blood's audiolib runs tasks on two channels, the timer's and
# the music's; Blood's clients are the timer's
edit('source/blood/satimer.cpp', [
    ('    client[nClients].pTask = TS_ScheduleTask((task_cast)pCall, nRate, 1, NULL);',
     '    client[nClients].pTask = TS_ScheduleTask((task_cast)pCall, nRate, 1, NULL, TASK_CHANNEL_TIMER);  // c4: chocolate-blood\'s audiolib'),
])
# the engine's voxel loader is a hook too (replace.cpp sets it)
edit('source/blood/build.h', [
    ('extern void (*uninitengine)(void);',
     'extern void (*uninitengine)(void);\nextern void (*loadvoxel)(int32_t);  // c4: tile.cpp\'s loadvoxel_replace'),
])
print('ok')
