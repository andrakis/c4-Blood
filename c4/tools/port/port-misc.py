#!/usr/bin/env python3
# port-misc.py SRCDIR: smaller ports of Blood-RE's game code to the host
# and c4 (each with its reason), applied once.
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

# Watcom had no bool keyword
edit('config.cpp', [
    ('void SCRIPT_GetBool(int32 scripthandle, char *section, char *entryname, BOOL *bool)\n{\n    boolean temp = *bool;',
     'void SCRIPT_GetBool(int32 scripthandle, char *section, char *entryname, BOOL *pBool)\n{\n    boolean temp = *pBool;'),
    ('    *bool = temp;', '    *pBool = temp;'),
])

# main: the command line through the platform (Watcom gave _argc and _argv),
# the engine's hooks set first, the platform's clock and sound started, as
# chocolate-blood's main does; no DOS file handles to grow
edit('blood.cpp', [
    ('void main(void)\n{\n    CheckIfWindows();\n    if (_grow_handles(40) < 40)\n        ThrowError(2089+LDIFF6)("Not enough file handles available.\\nIncrease FILES=## value in CONFIG.SYS.");',
     'void replace_hook();\n\n'
     'int main(int argc, char **argv)\n{\n'
     '    // c4: the engine calls Blood\'s replacements through pointers (replace.cpp),\n'
     '    // and the platform (system.h, sound.h) takes the PC\'s place\n'
     '    replace_hook();\n'
     '    Sys_Init(argc, argv);\n'
     '    Sound_Init(44100);\n'
     '    CheckIfWindows();'),
])
print('ok')
