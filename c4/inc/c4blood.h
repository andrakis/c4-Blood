// c4blood.h -- what MSVC's and Watcom's C libraries gave chocolate-blood that
// a Unix libc does not: forced into every unit (gcc -include; the c4lc build
// will include it from compat.h). The functions are in c4/c4blood.c.
#ifndef C4BLOOD_H
#define C4BLOOD_H 1
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>	// _exit, which MSVC has in stdlib.h
#include <errno.h>	// MSVC's stdlib.h declares errno
#include "dos.h"	// struct find_t, where Watcom code used it without dos.h
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _MAX_PATH
#define _MAX_PATH  144  // Watcom's (DOS): GAMEOPTIONS, in demos and saves, holds two
#define _MAX_DRIVE 3
#define _MAX_DIR   256
#define _MAX_FNAME 256
#define _MAX_EXT   256
#endif
#ifndef _MAX_PATH2
#define _MAX_PATH2 (_MAX_PATH + 3)  // Watcom's, for _splitpath2's buffer
#endif

#define stricmp   strcasecmp
#define _stricmp  strcasecmp
#define strnicmp  strncasecmp
#define _strnicmp strncasecmp
char *strupr (char *s);
char *strlwr (char *s);
char *itoa (int value, char *buf, int radix);
char *ltoa (long value, char *buf, int radix);
void _makepath (char *path, const char *drive, const char *dir, const char *fname, const char *ext);
void _splitpath (const char *path, char *drive, char *dir, char *fname, char *ext);
// Watcom's: pointers into BUF instead of copies
void _splitpath2 (const char *path, char *buf, const char **drive, const char **dir,
                  const char **fname, const char **ext);
long filelength (int fd);
#define strcmpi   strcasecmp
#define _msize(p) c4blood_msize (p)
unsigned long c4blood_msize (void *p);
// Watcom's and MSVC's stdlib.h had these as macros (C only: C++ units
// may define their own)
#ifndef __cplusplus
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#endif
// units that call these without including system.h (CACHE1D.C)
void sys_puts (const char *s);
void sys_printf (const char *s, ...);
void Sys_WaitVSync (void);  // system.c has it, system.h does not
// Watcom BIOS calls in Blood-RE's misc.h (int 0x10): the platform's
void setvmode (int mode);
int getvmode (void);
#ifdef C4BLOOD_C4
// C4IX's libc has no div_t (VES2.H declares one it never uses)
typedef struct { int quot, rem; } div_t;
#endif

// DOS's file names have no case, and Blood spells them either way
// (tables.dat for TABLES.DAT): a name that is not found as spelled is
// looked for without case in its directory. Backslashes are slashes.
int c4blood_open (const char *path, int flags, ...);
FILE *c4blood_fopen (const char *path, const char *mode);
int c4blood_access (const char *path, int mode);
#ifndef C4BLOOD_IMPL
#define open   c4blood_open
#define fopen  c4blood_fopen
#define access c4blood_access
#endif

#ifdef __cplusplus
}
#endif
#endif
