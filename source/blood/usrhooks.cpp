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
#include "typedefs.h"
extern "C" {
extern "C" {  // c4: audiolib, which calls these, is C
#include "USRHOOKS.H"
}
}
#include "resource.h"
#pragma pack(push, 1)  // c4: Watcom's packing, as Blood was built (packall.py)

int USRHOOKS_GetMem(void **ptr, uint32_t size)
{
    void *mem = Resource::Alloc(size);
    if (!mem)
        return -1;

    *ptr = mem;
    return 0;
}

int USRHOOKS_FreeMem(void *ptr)
{
    if (!ptr)
        return -1;
    Resource::Free(ptr);
    return 0;
}
#pragma pack(pop)  // c4: packall.py
