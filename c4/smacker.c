// smacker.c -- smack.h and svga.h without RAD's library: no movie opens.
#include "smack.h"
#include "svga.h"

void (*SmackTimerSetupAddr)(void);
u32 (*SmackTimerReadAddr)(void);
void (*SmackTimerDoneAddr)(void);

Smack *SmackOpen (const char *name, u32 flags, u32 extrabuf)
{
	(void)name; (void)flags; (void)extrabuf;
	return 0;
}
void SmackClose (Smack *smk) { (void)smk; }
u32 SmackDoFrame (Smack *smk) { (void)smk; return 0; }
void SmackNextFrame (Smack *smk) { (void)smk; }
u32 SmackWait (Smack *smk) { (void)smk; return 0; }
void SmackToScreen (Smack *smk, u32 left, u32 top, u32 bytesperline, const void *wintbl, void *setbank)
{
	(void)smk; (void)left; (void)top; (void)bytesperline; (void)wintbl; (void)setbank;
}

unsigned int WidthToUse = 320, HeightToUse = 200;
unsigned int SVGADetect (unsigned int flags) { (void)flags; return 0; }
unsigned int SVGASetup (unsigned int width, unsigned int height) { (void)width; (void)height; return 0; }
void SVGASetGraph (void) {}
void SVGASetText (void) {}
void SVGASetPalette (void *pal) { (void)pal; }
unsigned int SVGABytesPS (void) { return 0; }
void *SVGAWinTbl (void) { return 0; }
void *SVGASetBank (void) { return 0; }

u32 mult64anddiv (u32 m1, u32 m2, u32 d)
{
	return (u32)((unsigned long long)m1 * m2 / d);
}
