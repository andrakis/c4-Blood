/*
 * Copyright (C) 2018, 2022 nukeykt
 *
 * This file is part of Blood-RE.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _QHEAP_H_
#define _QHEAP_H_

#include "typedefs.h"

struct HEAPNODE;

class QHeap
{
public:
    QHeap(int heapSize);
    ~QHeap(void);

    void Check(void);
    void Debug(void);
    void *Alloc(int);
    int Free(void *p);

    void *heapPtr;
    void *extraPtr;
    void* AllocExtra(int size)
    {
        char* p = (char*)extraPtr;
        extraPtr = (void*)(p + size);

        return p;
    }
    HEAPNODE *heap;
    HEAPNODE *freeHeap;
    int size;
};

extern void* basePtr;

#if INTPTR_MAX > INT32_MAX  // c4: a 64-bit host only
#define QPTR(T) QPtr<T>
template<typename Type> struct QPtr
{
    uint32_t offset;

    Type* operator =(Type* ptr) { if (ptr) offset = (uint32_t)((char*)ptr - (char*)basePtr + 1); else offset = 0; return ptr; }
    operator Type*() { if (offset) return (Type*)((char*)basePtr + offset - 1); else return (Type*)nullptr; }
};
#else
#define QPTR(T) T *
#endif

#pragma pack(push, 1)

struct HEAPNODE
{
    QPTR(HEAPNODE) prev;
    QPTR(HEAPNODE) next;
    int size;
    QBOOL isFree;
    QPTR(HEAPNODE) freePrev;
    QPTR(HEAPNODE) freeNext;
};

#pragma pack(pop)

#endif
