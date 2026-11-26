// smack.h -- the part of RAD's Smacker SDK (3.x, Watcom) that Blood-RE's
// smkplay.cpp and satimer.cpp name. There is no Smacker decoder here:
// SmackOpen fails, so a cutscene is skipped, as the game does when its CD
// movies are missing. The timer hooks are kept, for satimer.cpp to set.
#ifndef C4BLOOD_SMACK_H
#define C4BLOOD_SMACK_H 1
#ifdef __cplusplus
extern "C" {
#endif

#ifndef pascal
#define pascal
#endif
// RAD's calling-convention and pointer-size macros (smkplay.cpp defines
// radmalloc and radfree with them)
#define RCFUNC
#define RADLINK
#define PTR4
typedef unsigned int u32;
typedef unsigned char u8;

typedef struct Smack {
	u32 Version, Width, Height, Frames, MSPerFrame, SmackerType;
	u32 LargestInTrack[7], tablesize, codesize, absize, detailsize, typesize;
	u32 TrackType[7], extra, NewPalette;
	u8 Palette[772];
	u32 PalType, FrameNum, FrameSize, SndSize;
} Smack;

Smack *SmackOpen (const char *name, u32 flags, u32 extrabuf);
void SmackClose (Smack *smk);
u32 SmackDoFrame (Smack *smk);
void SmackNextFrame (Smack *smk);
u32 SmackWait (Smack *smk);
void SmackToScreen (Smack *smk, u32 left, u32 top, u32 bytesperline, const void *wintbl, void *setbank);

// RAD.H's: m1 * m2 / d without overflow
u32 mult64anddiv (u32 m1, u32 m2, u32 d);

extern void (*SmackTimerSetupAddr)(void);
extern u32 (*SmackTimerReadAddr)(void);
extern void (*SmackTimerDoneAddr)(void);

#ifdef __cplusplus
}
#endif
#endif
