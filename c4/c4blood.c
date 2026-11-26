// c4blood.c -- the MSVC/Watcom C library calls chocolate-blood makes that a
// Unix libc lacks (c4blood.h, dos.h, conio.h): string case, number
// formatting, path splitting, file length, and DOS's file search. Plain C
// for gcc on the host and, later, c4lc.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include "c4blood.h"
#include "dos.h"
#include "conio.h"
#include "i86.h"
// the real calls here (c4blood.h makes the game's open c4blood_open)
#undef open
#undef fopen
#undef access

// ---- file names without case ----

// PATH as spelled, with slashes; or, when that does not exist, the entry of
// its directory that matches its last part without case. 0 if none.
static int find_nocase (const char *path, char *out, int n)
{
	char dir[600];
	const char *base;
	DIR *d;
	struct dirent *e;
	int i, k = 0;
	struct stat st;
	for (i = 0; path[i] && i < n - 1; i++)
		out[i] = path[i] == '\\' ? '/' : path[i];
	out[i] = 0;
	if (stat (out, &st) == 0)
		return 1;
	for (i = 0; out[i]; i++)
		if (out[i] == '/')
			k = i + 1;
	memcpy (dir, out, k);
	dir[k] = 0;
	base = out + k;
	if (!(d = opendir (k ? dir : ".")))
		return 0;
	while ((e = readdir (d)))
		if (!strcasecmp (e->d_name, base)) {
			snprintf (out, n, "%s%s", dir, e->d_name);
			closedir (d);
			return 1;
		}
	closedir (d);
	return 0;
}

int c4blood_open (const char *path, int flags, ...)
{
	char real[600];
	int mode = 0;
	if (flags & O_CREAT) {
		va_list vl;
		va_start (vl, flags);
		mode = va_arg (vl, int);
		va_end (vl);
	}
	if (!find_nocase (path, real, sizeof real))
		snprintf (real, sizeof real, "%s", path);
	return open (real, flags, mode);
}

FILE *c4blood_fopen (const char *path, const char *mode)
{
	char real[600];
	if (!find_nocase (path, real, sizeof real))
		snprintf (real, sizeof real, "%s", path);
	return fopen (real, mode);
}

int c4blood_access (const char *path, int mode)
{
	char real[600];
	if (!find_nocase (path, real, sizeof real))
		return -1;
	return access (real, mode);
}

char *strupr (char *s)
{
	char *p;
	for (p = s; *p; p++)
		*p = toupper ((unsigned char)*p);
	return s;
}

char *strlwr (char *s)
{
	char *p;
	for (p = s; *p; p++)
		*p = tolower ((unsigned char)*p);
	return s;
}

static char *utoa_base (unsigned long v, char *buf, int radix, int neg)
{
	char tmp[40];
	int n = 0;
	char *p = buf;
	if (radix < 2 || radix > 36)
		radix = 10;
	do {
		int d = (int)(v % radix);
		tmp[n++] = d < 10 ? '0' + d : 'a' + d - 10;
		v /= radix;
	} while (v);
	if (neg)
		*p++ = '-';
	while (n)
		*p++ = tmp[--n];
	*p = 0;
	return buf;
}

// as MSVC: a negative value is signed only in base 10
char *itoa (int value, char *buf, int radix)
{
	if (radix == 10 && value < 0)
		return utoa_base ((unsigned long)(-(long)value), buf, radix, 1);
	return utoa_base ((unsigned int)value, buf, radix, 0);
}

char *ltoa (long value, char *buf, int radix)
{
	if (radix == 10 && value < 0)
		return utoa_base ((unsigned long)(-value), buf, radix, 1);
	return utoa_base ((unsigned long)value, buf, radix, 0);
}

void _makepath (char *path, const char *drive, const char *dir, const char *fname, const char *ext)
{
	size_t n;
	*path = 0;
	if (drive && *drive) {
		strcat (path, drive);
		if (drive[strlen (drive) - 1] != ':')
			strcat (path, ":");
	}
	if (dir && *dir) {
		strcat (path, dir);
		n = strlen (dir);
		if (dir[n - 1] != '/' && dir[n - 1] != '\\')
			strcat (path, "/");
	}
	if (fname)
		strcat (path, fname);
	if (ext && *ext) {
		if (*ext != '.')
			strcat (path, ".");
		strcat (path, ext);
	}
}

// Split PATH: the directory part ends at the last slash, the extension
// starts at the last dot after it. No drives on Unix.
static void split (const char *path, int *dirend, int *extstart)
{
	int i, n = (int)strlen (path);
	*dirend = 0;
	for (i = 0; i < n; i++)
		if (path[i] == '/' || path[i] == '\\')
			*dirend = i + 1;
	*extstart = n;
	for (i = n - 1; i >= *dirend; i--)
		if (path[i] == '.') {
			*extstart = i;
			break;
		}
}

void _splitpath (const char *path, char *drive, char *dir, char *fname, char *ext)
{
	int d, e;
	split (path, &d, &e);
	if (drive)
		*drive = 0;
	if (dir) {
		memcpy (dir, path, d);
		dir[d] = 0;
	}
	if (fname) {
		memcpy (fname, path + d, e - d);
		fname[e - d] = 0;
	}
	if (ext)
		strcpy (ext, path + e);
}

// Watcom's: BUF gets the four parts, each NUL-ended, and the pointers point
// at them.
void _splitpath2 (const char *path, char *buf, const char **drive, const char **dir,
                  const char **fname, const char **ext)
{
	int d, e;
	char *p = buf;
	split (path, &d, &e);
	*p = 0;
	if (drive)
		*drive = p;
	p++;
	if (dir)
		*dir = p;
	memcpy (p, path, d);
	p[d] = 0;
	p += d + 1;
	if (fname)
		*fname = p;
	memcpy (p, path + d, e - d);
	p[e - d] = 0;
	p += e - d + 1;
	if (ext)
		*ext = p;
	strcpy (p, path + e);
}

long filelength (int fd)
{
	struct stat st;
	if (fstat (fd, &st) < 0)
		return -1;
	return (long)st.st_size;
}

// ---- DOS's file search: * and ?, without case, as DOS matched ----

static int match (const char *p, const char *s)
{
	for (; *p; p++, s++) {
		if (*p == '*') {
			// DOS's "*.*" also matches a name with no dot
			if (p[1] == '.' && p[2] == '*' && p[3] == 0)
				return 1;
			for (;; s++) {
				if (match (p + 1, s))
					return 1;
				if (!*s)
					return 0;
			}
		}
		if (!*s)
			return 0;
		if (*p != '?' && tolower ((unsigned char)*p) != tolower ((unsigned char)*s))
			return 0;
	}
	return *s == 0;
}

static unsigned findnext (struct find_t *f)
{
	struct dirent *e;
	struct stat st;
	char full[600];
	if (!f->dir)
		return 1;
	while ((e = readdir ((DIR *)f->dir))) {
		if (!match (f->pattern, e->d_name))
			continue;
		snprintf (full, sizeof full, "%s%s", f->dirname, e->d_name);
		if (stat (full, &st) < 0 || S_ISDIR (st.st_mode))
			continue;
		strncpy (f->name, e->d_name, sizeof f->name - 1);
		f->name[sizeof f->name - 1] = 0;
		f->size = (unsigned long)st.st_size;
		f->attrib = _A_NORMAL;
		return 0;
	}
	closedir ((DIR *)f->dir);
	f->dir = 0;
	return 1;
}

unsigned _dos_findfirst (const char *pattern, unsigned attrib, struct find_t *f)
{
	int d, e;
	(void)attrib;
	split (pattern, &d, &e);
	memset (f, 0, sizeof *f);
	memcpy (f->dirname, pattern, d);
	f->dirname[d] = 0;
	// DOS's separators: ".\" is "./"
	for (e = 0; f->dirname[e]; e++)
		if (f->dirname[e] == '\\')
			f->dirname[e] = '/';
	strncpy (f->pattern, pattern + d, sizeof f->pattern - 1);
	f->dir = opendir (d ? f->dirname : ".");
	return findnext (f);
}

unsigned _dos_findnext (struct find_t *f)
{
	return findnext (f);
}

unsigned _dos_findclose (struct find_t *f)
{
	if (f->dir)
		closedir ((DIR *)f->dir);
	f->dir = 0;
	return 0;
}

void _dos_getdrive (unsigned *drive)
{
	*drive = 3;
}

void _dos_setdrive (unsigned drive, unsigned *total)
{
	(void)drive;
	if (total)
		*total = 26;
}

int kbhit (void)
{
	return 0;
}

int getch (void)
{
	return '\r';
}

// MSVC's _msize (mact's SafeMallocSize): the host's malloc can say; on c4
// the libc keeps no sizes it can give, so 0 (nothing in Blood calls it).
#ifdef C4BLOOD_C4
unsigned long c4blood_msize (void *p)
{
	(void)p;
	return 0;
}
#else
#include <malloc.h>
unsigned long c4blood_msize (void *p)
{
	return (unsigned long)malloc_usable_size (p);
}
#endif

// ---- Watcom's DOS calls: no DOS here (i86.h) ----

int int386 (int n, union REGS *in, union REGS *out)
{
	(void)n;
	if (out != in)
		*out = *in;
	out->x.cflag = 1;
	return (int)out->x.eax;
}

int int386x (int n, union REGS *in, union REGS *out, struct SREGS *s)
{
	(void)s;
	return int386 (n, in, out);
}

void segread (struct SREGS *s)
{
	memset (s, 0, sizeof *s);
}
