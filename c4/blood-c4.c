// blood-c4.c -- c4's platform for Blood with a display and a sound card:
// blood-host.c's PC (the PIT, the keyboard interrupt, the VGA page and the
// checksums) built with C4BLOOD_SHOW, and the hooks it then calls, here.
//
// Played, the time is the machine's (bp_ms), the pages go to the display
// (blood-gui.c, with blood-c4ix.c or blood-bare.c), the keys come from it
// as the PC's scan codes, into the keyboard interrupt, and the mouse as
// the motion and buttons Int 33h gave; the sound is blood-snd.c's. Given
// -sum, -frames, -shot or -keys, it is a test: blood-host.c's virtual
// time and checksums, as B0 has them, with the pages shown only when
// -show is given too, and no sound, so that a test plays as B0 does.
#define C4BLOOD_SHOW 1
#include "blood-host.c"
#include "blood-plat.h"
#include "accel.h"
#include "c4accel.h"

static int show;			// the pages go to the display
static int trace;			// -trace: a line a second of the time and the frames
static uint32_t last_trace;
static uint32_t ms0;			// the machine's clock at the start
static uint32_t last_tick;
static int pal32[256];
static char pal_was[768];

static int want_show, want_window;

static int bh_test (void)
{
	return dosum || stopat || shot || keys;
}

// c4's own switches, taken out of argv (Blood stops on one it does not know):
//   -show     a test's pages drawn on the display too
//   -window   played in a window (C4IX's Terminal, grin's), not on the display
//   -trace    a line a second: the machine's time, the frames, the game's clock
static int bh_own (const char *arg)
{
	if (!strcmp (arg, "-show"))
		want_show = 1;
	else if (!strcmp (arg, "-window") || !strcmp (arg, "--window"))
		want_window = 1;
	else if (!strcmp (arg, "-trace"))
		trace = 1;
	else
		return 0;
	return 1;
}

static void bh_args (void)
{
	bh_real = !bh_test ();
	show = bh_real || want_show;
	ms0 = bp_ms ();
	// the 64-bit arithmetic by Shadow Warrior's accelerators, and the
	// engine's drawing loops by Blood's own (c4/inc/c4accel.h), where the
	// machine has them: played or a test, which then holds them to B0
	blood_accel = accel_find ("sw");
	blood_draw = accel_find ("blood");
	if (show)
		bp_init (want_window);
	if (bh_real)
		bs_init ();
	// where the game's files are, for a system that starts it elsewhere
	bp_chdir ();
}

static uint32_t last_ms;

static int bh_moved (void)
{
	uint32_t now = bp_ms ();
	if (now == last_ms)
		return 0;
	last_ms = now;
	return 1;
}

static uint64_t bh_now_ns (void)
{
	return (uint64_t)(uint32_t)(bp_ms () - ms0) * 1000000ull;
}

static void bh_sleep_ns (uint64_t ns)
{
	int ms = (int)(ns / 1000000);
	if (ms > 0)
		bp_sleep (ms);
}

// ---- the keys and the mouse ----

// A browser's key code as the PC's scan code: 0xE0nn for an extended key,
// 0 for one the game has no use for (Duke Nukem 3D's port's table)
static unsigned int scancodes[256] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0-7
	0x0E, 0x0F, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00,	// 8-15
	0x2A, 0x1D, 0x38, 0x59, 0x3A, 0x00, 0x00, 0x00,	// 16-23
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,	// 24-31
	0x39, 0xE049, 0xE051, 0xE04F, 0xE047, 0xE04B, 0xE048, 0xE04D,	// 32-39
	0xE050, 0x00, 0x00, 0x00, 0x00, 0xE052, 0xE053, 0x00,	// 40-47
	0x0B, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,	// 48-55
	0x09, 0x0A, 0x00, 0x27, 0x00, 0x0D, 0x00, 0x00,	// 56-63
	0x00, 0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22,	// 64-71
	0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18,	// 72-79
	0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11,	// 80-87
	0x2D, 0x15, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00,	// 88-95
	0x52, 0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47,	// 96-103
	0x48, 0x49, 0x37, 0x4E, 0x00, 0x4A, 0x53, 0xE035,	// 104-111
	0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42,	// 112-119
	0x43, 0x44, 0x57, 0x58, 0x00, 0x00, 0x00, 0x00,	// 120-127
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 128-135
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 136-143
	0x45, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 144-151
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 152-159
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 160-167
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00,	// 168-175
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 176-183
	0x00, 0x00, 0x27, 0x0D, 0x33, 0x0C, 0x34, 0x35,	// 184-191
	0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 192-199
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 200-207
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 208-215
	0x00, 0x00, 0x00, 0x1A, 0x2B, 0x1B, 0x28, 0x00,	// 216-223
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 224-231
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 232-239
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 240-247
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 248-255
};

// one byte into the keyboard interrupt
static void kb (int b)
{
	if (!kb_callback)
		return;
	kb_byte = (char)b;
	kb_callback ();
}

static void key (int code, int down)
{
	unsigned int sc;
	sc = code >= 0 && code < 256 ? scancodes[code] : 0;
	if (sc == 0)
		return;
	// Pause, which a PC sends as E1 1D 45 E1 9D C5 and never lets go
	if (sc == 0x59) {
		if (down) {
			kb (0xe1); kb (0x1d); kb (0x45); kb (0xe1); kb (0x9d); kb (0xc5);
		}
		return;
	}
	if (sc & 0xff00)
		kb (0xe0);
	kb ((sc & 0x7f) | (down ? 0 : 0x80));
}

// Int 33h's buttons: 1 left, 2 right, 4 middle
static void button (int b, int down)
{
	int bit = b == 0 ? 1 : b == 2 ? 2 : 4;
	if (down)
		bh_mouse_buttons |= bit;
	else
		bh_mouse_buttons &= ~bit;
}

// A key or button let go in the frame it went down in is let go only after
// the whole of the next frame: the game asks what is down once a frame,
// and a quick tap's press and release can both come inside one (Duke
// Nukem 3D's port, where a DOS keyboard's interrupt made that rare)
enum { HELD = 16 };
static int held[HELD], heldat[HELD], nheld;	// key codes, or 1000 + a button
static long downat[256 + 8];

static void hold (int what, long at)
{
	if (nheld < HELD) {
		held[nheld] = what;
		heldat[nheld++] = (int)at;
	} else if (what >= 1000)
		button (what - 1000, 0);
	else
		key (what, 0);
}

static int grabbed;

static void events (void)
{
	int ev[4], i, n, code;
	for (i = n = 0; i < nheld; i++)
		if (heldat[i] < frames - 1) {
			if (held[i] >= 1000)
				button (held[i] - 1000, 0);
			else
				key (held[i], 0);
		} else {
			held[n] = held[i];
			heldat[n++] = heldat[i];
		}
	nheld = n;
	while (bp_poll (ev)) {
		switch (ev[0]) {
		case BP_EV_KEYDOWN:
			downat[ev[1] & 255] = frames;
			key (ev[1], 1);
			break;
		case BP_EV_KEYUP:
			code = ev[1] & 255;
			if (downat[code] >= frames - 1)
				hold (ev[1], downat[code]);
			else
				key (ev[1], 0);
			break;
		case BP_EV_REL:
			bh_mouse_dx += ev[1];
			bh_mouse_dy += ev[2];
			break;
		case BP_EV_DOWN:
			downat[256 + (ev[3] & 7)] = frames;
			button (ev[3] & 7, 1);
			if (!grabbed) {
				bp_grab (1);
				grabbed = 1;
			}
			break;
		case BP_EV_UP:
			if (downat[256 + (ev[3] & 7)] >= frames - 1)
				hold (1000 + (ev[3] & 7), downat[256 + (ev[3] & 7)]);
			else
				button (ev[3] & 7, 0);
			break;
		case BP_EV_QUIT:
			// the window closed: the game ends, as when it is quit
			exit (0);
		}
	}
}

// the display's events and the music, at most every 4 ms of the machine's
// time: the game reads its clocks far more often than that
static void bh_tick (void)
{
	uint32_t now;
	if (!show && !bh_real)
		return;
	now = bp_ms ();
	if (trace && now - last_trace >= 1000) {
		last_trace = now;
		printf ("blood: %u ms, %ld frames, %u timer interrupts, %u music ticks in %u polls, %d ahead
",
		        (unsigned)(now - ms0), frames, bh_pits, bs_ticks, bs_polls, bh_real ? snd_ahead_us () : 0);
	}
	if (now - last_tick < 4)
		return;
	last_tick = now;
	if (show)
		events ();
	if (bh_real)
		bs_poll ();
}

// ---- the display ----

static void bh_mode (int w, int h)
{
	if (show)
		bp_mode (w, h);
}

static void bh_frame (const unsigned char *px)
{
	int i;
	if (!show)
		return;
	// the palette, 6 bits a component, as the display's 0xRRGGBB
	if (memcmp (pal_was, video_palette, 768)) {
		memcpy (pal_was, video_palette, 768);
		for (i = 0; i < 256; i++) {
			const unsigned char *c = (const unsigned char *)video_palette + i * 3;
			int r = (c[0] & 63) << 2, g = (c[1] & 63) << 2, b = (c[2] & 63) << 2;
			pal32[i] = ((r | r >> 6) << 16) | ((g | g >> 6) << 8) | (b | b >> 6);
		}
	}
	bp_frame ((uint8_t *)px, video_xdim, video_ydim, pal32);
	events ();
	if (bh_real)
		bs_poll ();
}
