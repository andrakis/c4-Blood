// blood-plat.h -- c4's port: what blood-c4.c (the PC the game sees, with
// a display) wants of the display and of the sound card:
//
//   blood-gui.c    libjs's display, with blood-c4ix.c or blood-bare.c
//   blood-snd.c    the sound card: the effects on its sample channels,
//                  the music on its OPL
#ifndef C4BLOOD_PLAT_H
#define C4BLOOD_PLAT_H 1

#include <stdint.h>

// events, as libjs's display numbers them (gui.h), and one of its own:
// ev[0] the kind; a key's ev[1] is its browser key code, ev[2] its
// character (0 for none) and ev[3] the modifiers; a button's ev[3] is
// the button (0 left, 1 middle, 2 right), the wheel's its direction; the
// motion's ev[1] and ev[2]
enum {
	BP_EV_DOWN = 2, BP_EV_UP = 3, BP_EV_KEYDOWN = 4, BP_EV_KEYUP = 5,
	BP_EV_WHEEL = 8, BP_EV_REL = 9, BP_EV_QUIT = 100
};

// blood-gui.c
void     bp_init (int window);		// the display taken, or a window
void     bp_mode (int w, int h);		// the game's screen is now w x h
void     bp_frame (uint8_t *pixels, int w, int h, int *pal);
int      bp_poll (int *ev);			// 1 and ev[0..3], or 0 for none
void     bp_grab (int on);			// the mouse locked to the display
uint32_t bp_ms (void);
void     bp_sleep (int ms);
// blood-c4ix.c or blood-bare.c: into the game's directory
void     bp_chdir (void);

// blood-snd.c
void     bs_init (void);
void     bs_poll (void);			// sounds let go, and the music run ahead
extern unsigned bs_ticks, bs_polls;	// the music's ticks and the polls, for -trace
int      snd_ahead_us (void);			// how far the card's stream is ahead, for -trace

#endif
