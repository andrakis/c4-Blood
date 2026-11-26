// blood-host.c -- chocolate-blood's platform (build/system.c, video.c,
// sound.c) on a host with gcc/g++ -m32, one thread, no display, no sound:
// the reference a c4 build is held to.
//
// chocolate-blood's SDL layer emulates a PC: a PIT whose interrupt
// (Sys_SetTimer) a second thread runs, a keyboard interrupt fed scancodes
// (kb_byte), a VGA frame buffer the thread shows at 70 Hz, a Sound Blaster
// and an AdLib. Here there is one thread and the time is virtual: it moves
// a millisecond each time the game reads a clock (SDL_GetAtomicInt on
// totalclock or gGameClock, shim SDL3/SDL.h) and by what SDL_Delay and
// Sys_WaitVSync wait, and the PIT's interrupts due by then run at that
// moment. So a run is the same on every machine and in every build.
//
// Every page the game shows (Video_BlitPage) is a frame: it is checksummed
// with the palette. Options, taken out of argv before the game sees them:
//   -sum N     a line every N frames (default 100), and the total at exit
//   -frames N  exit after N frames
//   -shot N    write every Nth frame as shotNNNNN.ppm
//   -keys F    play scancodes from file F: lines "FRAME SCANCODE" (hex
//              scancode; 0x80 set is a release), as the keyboard interrupt
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include "system.h"
#include "video.h"
#include "sound.h"

// ---- time ----

static uint64_t virt_ns;	// virtual time since start
static uint64_t pit_base_ns;
static uint64_t pit_cycles;
static int pit_divider;
static void (*pit_callback)(void);
static int in_pit;

static void PIT_Update (void)
{
	uint64_t cycles;
	if (in_pit)
		return;
	in_pit = 1;
	cycles = ((virt_ns - pit_base_ns) * 1193182ull) / 1000000000ull;
	if (pit_divider && pit_callback) {
		while (pit_cycles < cycles) {
			pit_callback ();
			pit_cycles += pit_divider;
		}
	} else
		pit_cycles = cycles;
	in_pit = 0;
}

void c4blood_clock_read (void)
{
	virt_ns += 1000000;
	PIT_Update ();
}

void c4blood_delay (int ms)
{
	virt_ns += (uint64_t)(ms > 0 ? ms : 1) * 1000000;
	PIT_Update ();
}

void Sys_SetTimer (int divider, void (*handler)(void))
{
	pit_base_ns = virt_ns;
	pit_cycles = 0;
	pit_divider = divider;
	pit_callback = handler;
}

// a vertical retrace at 70 Hz: wait for the next one
void Sys_WaitVSync (void)
{
	uint64_t frame = 1000000000ull / 70;
	virt_ns = (virt_ns / frame + 1) * frame;
	PIT_Update ();
}

// ---- keyboard and mouse ----

char kb_byte;
static void (*kb_callback)(void);
static FILE *keys;
static long key_frame = -1;
static int key_code;
static long frames;

static void next_key (void)
{
	if (!keys || fscanf (keys, "%ld %x", &key_frame, &key_code) != 2) {
		key_frame = -1;
		return;
	}
}

static void feed_keys (void)
{
	while (key_frame >= 0 && key_frame <= frames) {
		if (kb_callback) {
			if (key_code & 0x100) {
				kb_byte = (char)0xe0;
				kb_callback ();
			}
			kb_byte = (char)(key_code & 0xff);
			kb_callback ();
		}
		next_key ();
	}
}

void Sys_HandleEvents (void)
{
	PIT_Update ();
}

void Sys_SetKeyboardHandler (void (*handler)(void))
{
	kb_callback = handler;
}

void Sys_GetMouseDelta (float *dx, float *dy)
{
	*dx = 0;
	*dy = 0;
}

int Sys_GetMouseButtons (void)
{
	return 0;
}

// ---- text ----

void Video_Text_Puts (const char *s)
{
	(void)s;
}

void Video_Text_SetCursor (int x, int y)
{
	(void)x; (void)y;
}

void Video_Text_Scroll (int lines, int attr, int x1, int y1, int x2, int y2)
{
	(void)lines; (void)attr; (void)x1; (void)y1; (void)x2; (void)y2;
}

void sys_puts (const char *s)
{
	puts (s);
}

void sys_printf (const char *s, ...)
{
	va_list vl;
	va_start (vl, s);
	vprintf (s, vl);
	va_end (vl);
}

static int wseed = 1;
void wsrand (int seed)
{
	wseed = seed;
}

int wrand (void)
{
	wseed = 1103515245 * wseed + 12345;
	return (wseed >> 16) & WRAND_MAX;
}

// ---- the frame ----

char video_palette[768];
char *video_buffer;
static char *video_buffer2;
int video_pages;
int video_row_stride;
int video_page_stride;
int video_xdim, video_ydim;
int video_graphics;
char *video_text_buffer;

static int dosum = 100, stopat, shot;
static uint32_t total;

void Video_Init (void)
{
	video_text_buffer = calloc (80 * 25 * 2, 1);
}

void Video_Set (int graphics, int w, int h)
{
	video_graphics = graphics;
	if (graphics == 0x13) {
		free (video_buffer2);
		video_pages = 2;
		video_xdim = w;
		video_ydim = h;
		video_row_stride = w;
		video_page_stride = w * h;
		video_buffer2 = calloc (w * h * video_pages + 15, 1);
		video_buffer = (char *)((intptr_t)video_buffer2 & ~(intptr_t)15);
		printf ("blood-host: video %dx%d\n", w, h);
	}
}

static uint32_t fnv (uint32_t h, const unsigned char *p, int n)
{
	while (n--)
		h = (h ^ *p++) * 16777619u;
	return h;
}

static void write_shot (const unsigned char *px)
{
	char name[32];
	FILE *f;
	int i;
	snprintf (name, sizeof name, "shot%05ld.ppm", frames);
	if (!(f = fopen (name, "wb")))
		return;
	fprintf (f, "P6\n%d %d\n255\n", video_xdim, video_ydim);
	for (i = 0; i < video_xdim * video_ydim; i++) {
		const unsigned char *c = (const unsigned char *)video_palette + px[i] * 3;
		putc ((c[0] & 63) << 2, f);
		putc ((c[1] & 63) << 2, f);
		putc ((c[2] & 63) << 2, f);
	}
	fclose (f);
}

void Video_BlitPage (int32_t page)
{
	const unsigned char *px;
	uint32_t h;
	if (video_graphics != 0x13)
		return;
	px = (const unsigned char *)video_buffer + page * video_page_stride;
	h = fnv (2166136261u, px, video_xdim * video_ydim);
	h = fnv (h, (const unsigned char *)video_palette, 768);
	total = (total ^ h) * 16777619u + (uint32_t)frames;
	frames++;
	if (shot && frames % shot == 0)
		write_shot (px);
	if (dosum && frames % dosum == 0)
		printf ("blood-host: frame %ld sum %08x total %08x t=%llums\n", frames, h, total,
		        (unsigned long long)(virt_ns / 1000000));
	feed_keys ();
	if (stopat && frames >= stopat) {
		printf ("blood-host: stopped at frame %ld total %08x\n", frames, total);
		fflush (stdout);
		exit (0);
	}
}

void Video_Blit (void)
{
}

static void at_exit (void)
{
	printf ("blood-host: exit at frame %ld total %08x t=%llums\n", frames, total,
	        (unsigned long long)(virt_ns / 1000000));
}

// ---- sound: none (the game is told there is no card) ----

SDL_Mutex *snd_mutex;
int blaster_type = BLASTER_TYPE_NONE;
static char dma_page[65536];

void Sound_Init (int rate) { (void)rate; }
void Blaster_SetIrqHandler (void (*handler)(void)) { (void)handler; }
char *Blaster_SetDmaPageSize (int size) { (void)size; return dma_page; }
void Blaster_Init (int tc, int rate, int channels, int is16bit) { (void)tc; (void)rate; (void)channels; (void)is16bit; }
void Blaster_StartDma (int start, int dma_count, int count, int is_auto) { (void)start; (void)dma_count; (void)count; (void)is_auto; }
void Blaster_ContinueDma (int count) { (void)count; }
void Blaster_StopDma (void) {}
void Blaster_Shutdown (void) {}
int Blaster_GetDmaCount (void) { return 0; }
int Blaster_GetDmaPos (void) { return 0; }
int Blaster_GetVolume (void) { return 255; }
void Blaster_SetVolume (int vol) { (void)vol; }
int Music_GetVolume (void) { return 255; }
void Music_SetVolume (int vol) { (void)vol; }
void Music_SetTimer (int divider, void (*handler)(void)) { (void)divider; (void)handler; }
void Adlib_Write (int offset, int data) { (void)offset; (void)data; }

// ---- start ----

int32_t sys_argc;
char **sys_argv;

void Sys_Init (int argc, char **argv)
{
	int i, j;
	for (i = j = 1; i < argc; i++) {
		if (!strcmp (argv[i], "-sum") && i + 1 < argc)
			dosum = atoi (argv[++i]);
		else if (!strcmp (argv[i], "-frames") && i + 1 < argc)
			stopat = atoi (argv[++i]);
		else if (!strcmp (argv[i], "-shot") && i + 1 < argc)
			shot = atoi (argv[++i]);
		else if (!strcmp (argv[i], "-keys") && i + 1 < argc) {
			keys = fopen (argv[++i], "r");
			if (!keys)
				printf ("blood-host: no %s\n", argv[i]);
			next_key ();
		} else
			argv[j++] = argv[i];
	}
	argv[j] = 0;
	sys_argc = j;
	sys_argv = argv;
	setvbuf (stdout, 0, _IOLBF, 0);
	atexit (at_exit);
	Video_Init ();
	Video_Set (1, 720, 400);
}
