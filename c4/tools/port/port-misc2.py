#!/usr/bin/env python3
# port-misc2.py ROOT: more of the port, applied once (ROOT is the repo).
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

# QPtr keeps a 32-bit offset in place of a pointer, so that the cache's and
# heap's nodes keep their size on a 64-bit host. On a 32-bit one (-m32, c4)
# it is the pointer, and the template is not compiled.
edit('source/blood/qtools/include/qheap.h', [
    ('template<typename Type> struct QPtr\n{',
     '#if INTPTR_MAX > INT32_MAX  // c4: a 64-bit host only\n'
     '#define QPTR(T) QPtr<T>\n'
     'template<typename Type> struct QPtr\n{'),
    ('    operator Type*() { if (offset) return (Type*)((char*)basePtr + offset - 1); else return (Type*)nullptr; }\n};',
     '    operator Type*() { if (offset) return (Type*)((char*)basePtr + offset - 1); else return (Type*)nullptr; }\n};\n'
     '#else\n#define QPTR(T) T *\n#endif'),
    ('    QPtr<HEAPNODE> prev;\n    QPtr<HEAPNODE> next;', '    QPTR(HEAPNODE) prev;\n    QPTR(HEAPNODE) next;'),
    ('    QPtr<HEAPNODE>  freePrev;\n    QPtr<HEAPNODE>  freeNext;', '    QPTR(HEAPNODE) freePrev;\n    QPTR(HEAPNODE) freeNext;'),
])
edit('source/blood/qtools/include/resource.h', [
    ('    QPtr<void> ptr;\n    QPtr<CACHENODE> prev;\n    QPtr<CACHENODE> next;',
     '    QPTR(void) ptr;\n    QPTR(CACHENODE) prev;\n    QPTR(CACHENODE) next;'),
])

# chocolate-blood's engine takes the translucency table (its fixtransluscence
# does not read a global the game set)
edit('source/blood/screen.cpp', [
    ('    transluc = (byte*)gSysRes.Lock(pTrans);\n    fixtransluscence();',
     '    transluc = (byte*)gSysRes.Lock(pTrans);\n    fixtransluscence((intptr_t)transluc);  // c4: as chocolate-blood\'s engine takes it'),
])
print('ok')
