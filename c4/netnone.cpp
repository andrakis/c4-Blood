// netnone.cpp -- one player. Blood-RE's game calls into the network drivers
// DOS Blood was linked with: Mpath (func_83xxx in mpath1-3.cpp), HEAT
// (heat1-3.cpp), TEN (tenblood.cpp and its library) and engage.cpp. None is
// built here; these are what they do when there is no such network: each
// init fails (-1), each send sends nothing, and each receive has nothing.
// The same as chocolate-blood's stubs, kept apart from Blood-RE's sources.
// Also Watcom's DPMI memory for the CD-audio driver (cdrom.cpp).
#include "typedefs.h"

// blood.cpp
int func_83370(void) { return -1; }	// Mpath
int func_83D80(void) { return -1; }	// HEAT
int tenBloodInit(void) { return -1; }	// TEN
int func_835B0(void) { return -1; }
void func_86910(void) {}

// network.cpp: sending
void func_83444(int nDest, char *pBuffer, int nSize) { (void)nDest; (void)pBuffer; (void)nSize; }
short func_83E44(int nDest, char *pBuffer, int nSize) { (void)nDest; (void)pBuffer; (void)nSize; return 0; }
void func_8364C(short nDest, char *pBuffer, short nSize) { (void)nDest; (void)pBuffer; (void)nSize; }
int tenBloodSendPacket(int nIndex, char *bufptr, int buflen) { (void)nIndex; (void)bufptr; (void)buflen; return 0; }
byte func_86760(byte *p) { (void)p; return 0; }

// network.cpp: receiving
short func_8349C(short *pSource, char *pBuffer) { (void)pSource; (void)pBuffer; return 0; }
short func_83EB0(short *pSource, char *pBuffer) { (void)pSource; (void)pBuffer; return 0; }
short func_83700(short *pSource, char *pBuffer) { (void)pSource; (void)pBuffer; return 0; }
short tenBloodGetPacket(short *pSource, char *pBuffer) { (void)pSource; (void)pBuffer; return 0; }

// network.cpp: TEN's score, jitter and idle
int tenBloodScore(char *p) { (void)p; return 0; }
int func_83500(void) { return 0; }
void tenCalculateJitter(void) {}
void tenBloodIdle(void) {}

// view.cpp
void tenPlayerDebugInfo(char *a1, int pid) { (void)a1; (void)pid; }

// menu.cpp: TEN's launcher button (its library, C)
extern "C" void tenBnSetCommandLine(void (*a1)(char *)) { (void)a1; }
extern "C" int tenBnStart(void) { return 0; }

// cdrom.cpp: DOS memory below 1 MB for the CD driver's requests. The
// memory is real (the game fills it in whether or not there is a drive);
// the requests go nowhere, since int386 fails (i86.h). 0 is success.
#include <stdlib.h>
int dpmiAlloc(void **p, int *segment, int size)
{
	*p = calloc(1, size > 0 ? size : 1);
	*segment = 1;
	return *p ? 0 : 8;
}

int dpmiFree(int segment)
{
	(void)segment;	// the selector, not the address: kept until exit
	return 0;
}
