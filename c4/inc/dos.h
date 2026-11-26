// dos.h -- the parts of Watcom's dos.h chocolate-blood's DOS paths use:
// finding files (save games, demos, sound files, RFF overrides), and
// keywords and calls that are compiled out.
#ifndef C4BLOOD_DOS_H
#define C4BLOOD_DOS_H 1
#ifdef __cplusplus
extern "C" {
#endif

#define _A_NORMAL 0x00
#define _A_RDONLY 0x01
#define _A_HIDDEN 0x02
#define _A_SYSTEM 0x04
#define _A_SUBDIR 0x10
#define _A_ARCH   0x20

struct find_t {
	char reserved[21];
	char attrib;
	unsigned short wr_time;
	unsigned short wr_date;
	unsigned long size;
	char name[260];
	// ours: the search
	void *dir;
	char pattern[260];
	char dirname[260];
};

// 0 on a match (filled in), nonzero when there is none
unsigned _dos_findfirst (const char *pattern, unsigned attrib, struct find_t *f);
unsigned _dos_findnext (struct find_t *f);
unsigned _dos_findclose (struct find_t *f);
void _dos_getdrive (unsigned *drive);
void _dos_setdrive (unsigned drive, unsigned *total);

#ifndef cdecl
#define cdecl
#endif
#define _enable()
#define _disable()
#define _dos_getvect(n) ((void (*)(void))0)
#define _dos_setvect(n, f)

#ifdef __cplusplus
}
#endif
#endif
