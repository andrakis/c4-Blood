#!/usr/bin/env python3
# port-replace.py SRCDIR: Blood-RE overrides engine functions by defining
# their names (Watcom's linker took the game's over the library's).
# chocolate-blood's engine makes them hook pointers instead, set by
# replace_hook(): rename each definition NAME_replace, with the hook's
# signature, and add replace_hook() to replace.cpp, as chocolate-blood does.
import os, re, sys
d = sys.argv[1]
def edit(fn, pairs):
    p = os.path.join(d, fn)
    s = open(p).read()
    for old, new in pairs:
        if s.count(old) != 1:
            sys.exit('%s: expected one %r, found %d' % (fn, old, s.count(old)))
        s = s.replace(old, new)
    open(p, 'w').write(s)

edit('replace.cpp', [
    ('extern "C" void agecache()', 'extern "C" void agecache_replace()'),
    ('extern "C" void initcache()', 'extern "C" void initcache_replace(intptr_t, int32_t)'),
    ('extern "C" void allocache()', 'extern "C" void allocache_replace(intptr_t*, int32_t, char*)'),
    ('extern "C" void *kmalloc(long size)', 'extern "C" void *kmalloc_replace(size_t size)'),
    ('extern "C" void kfree(void *pMem)', 'extern "C" void kfree_replace(void *pMem)'),
    ('extern "C" int loadpics()', 'extern "C" int loadpics_replace(char*)'),
    ('extern "C" void loadtile(short nTile)', 'extern "C" void loadtile_replace(short nTile)'),
    ('extern "C" void allocatepermanenttile(short a1, int a2, int a3)\n{\n    tileAllocTile(a1, a2, a3);',
     'extern "C" intptr_t allocatepermanenttile_replace(short a1, int a2, int a3)\n{\n    return (intptr_t)tileAllocTile(a1, a2, a3);'),
    ('extern "C" int animateoffs(short nTile, ushort a2)', 'extern "C" int animateoffs_replace(short nTile, ushort a2)'),
    ('extern "C" void uninitengine()', 'extern "C" void uninitengine_replace()'),
    ('extern "C" void loadpalette()', 'extern "C" void loadpalette_replace()'),
    ('extern "C" int getpalookup(int a1, int a2)', 'extern "C" int getpalookup_replace(int a1, int a2)'),
])
p = os.path.join(d, 'replace.cpp')
s = open(p).read()
s += '''
// c4: chocolate-blood's engine (source/build) calls these through pointers;
// main sets them first, as chocolate-blood's does
extern "C"
{
    void loadvoxel_replace(int nVoxel);
    void initspritelists_replace(void);
    int insertsprite_replace(short a1, short a2);
    int deletesprite_replace(short a1);
    int changespritesect_replace(short nSprite, short nSector);
    int changespritestat_replace(short nSprite, short nStatus);

    extern void* (*kmalloc)(size_t);
    extern void (*kfree)(void*);
    extern void (*loadvoxel)(int32_t);
    extern void (*initcache)(intptr_t, int32_t);
    extern void (*allocache)(intptr_t*, int32_t, char*);
    extern void (*agecache)();
    extern int32_t(*loadpics)(char*);
    extern void (*loadtile)(short);
    extern intptr_t(*allocatepermanenttile)(short, int32_t, int32_t);
    extern void (*loadpalette)();
    extern int engine_revision;
}

void replace_hook()
{
    kmalloc = kmalloc_replace;
    kfree = kfree_replace;
    loadvoxel = loadvoxel_replace;
    initcache = initcache_replace;
    allocache = allocache_replace;
    agecache = agecache_replace;
    initspritelists = initspritelists_replace;
    insertsprite = insertsprite_replace;
    deletesprite = deletesprite_replace;
    changespritesect = changespritesect_replace;
    changespritestat = changespritestat_replace;
    loadpics = loadpics_replace;
    loadtile = loadtile_replace;
    allocatepermanenttile = allocatepermanenttile_replace;
    animateoffs = animateoffs_replace;
    uninitengine = uninitengine_replace;
    loadpalette = loadpalette_replace;
    getpalookup = getpalookup_replace;
    engine_revision = 19961006;  // Blood's engine (one library for every version)
}
'''
open(p, 'w').write(s)

edit('db.cpp', [
    ('\nvoid initspritelists(void)\n', '\nextern "C" void initspritelists_replace(void)\n'),
    ('\nint insertsprite(short a1, short a2)\n', '\nextern "C" int insertsprite_replace(short a1, short a2)\n'),
    ('\nvoid deletesprite(short a1)\n', '\nextern "C" int deletesprite_replace(short a1)\n'),
    ('\nint changespritesect(short nSprite, short nSector)\n', '\nextern "C" int changespritesect_replace(short nSprite, short nSector)\n'),
    ('\nint changespritestat(short nSprite, short nStatus)\n', '\nextern "C" int changespritestat_replace(short nSprite, short nStatus)\n'),
])
edit('tile.cpp', [
    ('extern "C" void loadvoxel(int nVoxel)', 'extern "C" void loadvoxel_replace(int nVoxel)'),
])
print('ok')
