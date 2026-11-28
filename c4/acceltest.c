// acceltest.c -- c4's port: Blood's accelerators (c4/accel.mjs, c4pc's
// games/blood.c) against the loops they stand for, which are a_asm.c's
// own: this includes a_asm.c and runs each case twice from the same start,
// blood_draw 0 (the C) and then the module's number (the accelerator), and
// holds the second to the first -- the pixels, what a loop returns, and
// the state it leaves (vplce, asm3, hlineasm4's step). Made-up textures,
// palettes, coordinates, steps and shifts, as the engine sets them up.
// test-blood runs it on c4's libjs, and test-c4pc on c4pc-host.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "accel.h"
#include "c4accel.h"

// what a_asm.c takes from the engine (ENGINE.C)
int32_t asm1, asm4;
intptr_t asm2, asm3;
int32_t reciptable[2048];
union { int32_t i; float f; } fpuasm;
int32_t globalx3, globaly3;
int32_t ylookup[1200];
int32_t vplce[4], vince[4];
intptr_t palookupoffse[4], bufplce[4];
char pow2char[8];
int32_t pow2long[32];
int blood_draw = -1;

#include "a_asm.c"

unsigned seed = 4242;
int rnd (int n) { seed = seed * 1103515245 + 12345; return (int)((seed >> 8) % (unsigned)n); }
int rnd32 (void) { return (rnd (65536) << 16) | rnd (65536); }

// a screen tall enough for mvlineasm4's blocks of 256 rows
enum { TEXN = 1 << 20, PALN = 65536 + 4096, W = 320, H = 720, BUFN = W * H };
unsigned char *tex, *pal, *bufc, *bufa;
int fails, cases, draw;

void fill (unsigned char *p, int n, int holes)
{
	int i;
	for (i = 0; i < n; i++) { p[i] = rnd (256); if (holes && rnd (6) == 0) p[i] = 255; }
}
void clear (void)
{
	int i;
	for (i = 0; i < BUFN; i++) bufc[i] = rnd (256);
	memcpy (bufa, bufc, BUFN);
}
void check (char *what, int k, int same)
{
	cases++;
	if (memcmp (bufc, bufa, BUFN) != 0 || !same)
	{
		fails++;
		if (fails < 12) printf ("accel: %s, case %d, differs%s\n", what, k, same ? "" : " (its state)");
	}
}

// the four columns: textures, palettes, places and steps
int32_t v0[4], i0[4];
intptr_t b0[4], p0[4];
void columns (int logy)
{
	int k;
	for (k = 0; k < 4; k++)
	{
		v0[k] = rnd32 (); i0[k] = rnd32 () >> rnd (8);
		b0[k] = (intptr_t)(tex + rnd (TEXN - (2 << logy) - 256));
		p0[k] = (intptr_t)(pal + 256 * rnd (32));
	}
}
void set_columns (void)
{
	memcpy (vplce, v0, sizeof vplce); memcpy (vince, i0, sizeof vince);
	memcpy (bufplce, b0, sizeof bufplce); memcpy (palookupoffse, p0, sizeof palookupoffse);
}

int main (void)
{
	int k, n, x, y, al, bl, cnt, logy, ebx, r1, r2;
	int32_t edx, esi, e1, e2, vp[4];
	intptr_t a3;
	uint64_t sub;
	unsigned char *base;
	draw = accel_find ("blood");
	if (draw <= 0)
	{
		printf ("accel: no accelerators here\n");
		return 1;
	}
	tex = malloc (TEXN + 8192); pal = malloc (PALN);
	// the two screens alike modulo 16: the loops' ways with edi's low bits
	base = malloc (2 * BUFN + 64);
	bufc = (unsigned char *)(((intptr_t)base + 15) & ~(intptr_t)15);
	bufa = bufc + ((BUFN + 31) & ~31);
	fill (tex, TEXN + 8192, 1); fill (pal, PALN, 0);
	for (y = 0; y < 1200; y++) ylookup[y] = y * W;
	setvlinebpl (W);

	// hlineasm4: the floors' rows, right to left, long and short, the
	// step from asm1 and asm2 (ebx >= 0) or the one set up before
	for (k = 0; k < 400; k++)
	{
		al = 1 + rnd (10); bl = 1 + rnd (10);
		sethlinesizes (al, bl, (intptr_t)(tex + rnd (4096)));
		setpalookupaddress ((char *)pal + rnd (256));
		setuphlineasm4 (rnd32 (), rnd32 ());
		asm1 = rnd32 () >> rnd (12); asm2 = rnd32 () >> rnd (12);
		cnt = k % 4 == 0 ? rnd (9) : rnd (300);
		ebx = rnd (2) ? rnd (1000) : -1 - rnd (1000);
		edx = rnd32 (); esi = rnd32 ();
		x = cnt + rnd (W - cnt); y = rnd (H);
		n = 256 * rnd (16);
		clear ();
		sub = hlineasm4_sub;
		blood_draw = 0;
		hlineasm4 (cnt, ebx, n, edx, esi, (intptr_t)(bufc + y * W + x));
		e1 = (int32_t)hlineasm4_sub; e2 = (int32_t)(hlineasm4_sub >> 32);
		hlineasm4_sub = sub;
		blood_draw = draw;
		hlineasm4 (cnt, ebx, n, edx, esi, (intptr_t)(bufa + y * W + x));
		check ("hlineasm4", k, e1 == (int32_t)hlineasm4_sub && e2 == (int32_t)(hlineasm4_sub >> 32));
	}

	// vlineasm4 and mvlineasm4: four columns down, as the walls and the
	// masked walls draw them; long enough for mvlineasm4's blocks of 256
	for (k = 0; k < 400; k++)
	{
		int masked = k & 1;
		logy = 1 + rnd (10);
		columns (logy);
		x = 4 * rnd (W / 4 - 1);
		cnt = masked ? rnd (k % 8 == 1 ? 700 : H - 1) : rnd (H - 1);
		y = masked ? 0 : rnd (H - cnt);
		clear ();
		a3 = rnd32 ();
		if (masked)
		{
			setupmvlineasm (32 - logy);
			set_columns (); asm3 = a3; blood_draw = 0;
			mvlineasm4 (cnt, (intptr_t)(bufc + x));
			memcpy (vp, vplce, sizeof vp); e1 = (int32_t)asm3;
			set_columns (); asm3 = a3; blood_draw = draw;
			mvlineasm4 (cnt, (intptr_t)(bufa + x));
			check ("mvlineasm4", k, !memcmp (vp, vplce, sizeof vp) && e1 == (int32_t)asm3);
		}
		else
		{
			setupvlineasm (32 - logy);
			set_columns (); blood_draw = 0;
			vlineasm4 (cnt, (intptr_t)(bufc + y * W + x));
			memcpy (vp, vplce, sizeof vp);
			set_columns (); blood_draw = draw;
			vlineasm4 (cnt, (intptr_t)(bufa + y * W + x));
			check ("vlineasm4", k, !memcmp (vp, vplce, sizeof vp));
		}
	}

	// vlineasm1 and mvlineasm1: one column, and the coordinate after
	for (k = 0; k < 400; k++)
	{
		int masked = k & 1, inc;
		intptr_t tx, pp;
		logy = 1 + rnd (10);
		tx = (intptr_t)(tex + rnd (TEXN - (2 << logy)));
		pp = (intptr_t)(pal + rnd (4096));
		cnt = rnd (H - 1);
		x = rnd (W); y = rnd (H - cnt);
		edx = rnd32 (); inc = rnd32 () >> rnd (8);
		clear ();
		if (masked)
		{
			setupmvlineasm (32 - logy);
			blood_draw = 0;
			r1 = mvlineasm1 (inc, pp, cnt, edx, tx, (intptr_t)(bufc + y * W + x));
			blood_draw = draw;
			r2 = mvlineasm1 (inc, pp, cnt, edx, tx, (intptr_t)(bufa + y * W + x));
			check ("mvlineasm1", k, r1 == r2);
		}
		else
		{
			setupvlineasm (32 - logy);
			blood_draw = 0;
			r1 = vlineasm1 (inc, pp, cnt, edx, tx, (intptr_t)(bufc + y * W + x));
			blood_draw = draw;
			r2 = vlineasm1 (inc, pp, cnt, edx, tx, (intptr_t)(bufa + y * W + x));
			check ("vlineasm1", k, r1 == r2);
		}
	}

	if (fails)
		printf ("accel: Blood's, %d of %d cases differ from their loops\n", fails, cases);
	else
		printf ("accel: Blood's, %d cases, each as its loop has it\n", cases);
	return fails != 0;
}
