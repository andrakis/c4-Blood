// blood-snd.c -- Blood's sound on libjs's sound card (libjs/guest/snd.h),
// for c4's display build (blood-c4.c): audiolib's FX_ interface (FX_MAN.H)
// in place of its FX_MAN.C and the MultiVoc mixer behind it, and the
// platform's AdLib and music timer (build/sound.h) under audiolib's own
// MIDI player and AdLib driver. As Shadow Warrior's sw-snd.c and Duke
// Nukem 3D's duke-snd.c, which play Apogee's audiolib on the same card.
//
// The effects are SOUNDS.RFF's raw 8-bit sounds, and its VOC and WAV ones.
// They play as they are on the card's sixteen sample channels, at their
// own rate moved by the pitch the game asks for, panned as the game pans
// them. With all sixteen playing, a new sound takes the oldest of the
// lowest priority (one panned out of hearing first), as audiolib did. A
// looped sound plays again from its loop's start each time it ends.
//
// The music is the DOS game's own: audiolib's MIDI player (MIDI.C) and its
// AdLib driver (AL_MIDI.C), with Blood's timbre bank, on the card's OPL as
// an AdLib at 0x388 (MusicDevice 3 in BLOOD.CFG). The player is a task of
// audiolib's task manager (TASK_MAN.C) on its music channel, whose timer
// (Music_SetTimer) the SDL build ran by the OPL samples it made: here
// bs_poll() runs it a tick at a time with snd_wait() between ticks, so each
// write is stamped with the stream's time, until the card's stream is a
// quarter of a second ahead.
//
// The Sound Blaster itself is not there (its DMA is never started): with
// no card, or no OPL, the game plays silent rather than stopping, as it
// would on a DOS machine that had none.
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "FX_MAN.H"
#include "sound.h"
#include "snd.h"
#include "blood-plat.h"

enum { AHEAD_US = 250000, MAXSTEPS = 4000 };

static int card, opl, fx_on;
int FX_ErrorCode = FX_Ok;
enum { C4_NoCard = 100 };		// this file's own error: no card

// ---- the platform's Sound Blaster: none of it runs ----

SDL_Mutex *snd_mutex;
int blaster_type = BLASTER_TYPE_16;
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

// ---- the AdLib, and the music's timer ----

static int opl_address, opl_newm;
static int music_volume = 255;
static int music_divider;
static void (*music_callback)(void);
static uint32_t music_rem;		// the stream's time left over, in PIT cycles times 10^6

// AL_MIDI.C's port writes: an even port takes the register, an odd one its
// value; the second bank (0x38A) as the OPL3 has it
void Adlib_Write (int offset, int data)
{
	if ((offset & 1) == 0) {
		opl_address = data & 255;
		if ((offset & 2) != 0 && (opl_address == 5 || opl_newm))
			opl_address |= 0x100;
	} else {
		if (opl_address == 0x105)
			opl_newm = data & 1;
		if (opl)
			snd_opl (opl_address, data & 255);
	}
}

void Music_SetTimer (int divider, void (*handler)(void))
{
	music_divider = divider;
	music_callback = handler;
}

int Music_GetVolume (void) { return music_volume; }
void Music_SetVolume (int vol) { music_volume = vol; }

static void silence (void)
{
	int i;
	if (fx_on)
		snd_stop (0 - 1);
	if (!opl)
		return;
	// every channel's key off, and the card as an OPL2 for the next program
	for (i = 0; i < 9; i++) {
		snd_opl (0xb0 + i, 0);
		snd_opl (0x1b0 + i, 0);
	}
	snd_opl (0x104, 0);
	snd_opl (0x105, 0);
}

void bs_init (void)
{
	card = snd_present ();
	opl = card && (snd_caps () & SND_HAS_OPL);
	if (!card)
		return;
	snd_clock (1000000);
	if (opl) {
		snd_opl (0x104, 0);
		snd_opl (0x105, 0);
	}
	atexit (silence);
}

static void fx_reap (void);
unsigned bs_ticks, bs_polls;
int snd_ahead_us (void) { return card ? snd_ahead () : 0; }

void bs_poll (void)
{
	int n;
	uint64_t t;
	uint32_t us;
	if (!card)
		return;
	bs_polls++;
	if (fx_on)
		fx_reap ();
	if (!opl || !music_divider || !music_callback)
		return;
	for (n = 0; n < MAXSTEPS && snd_ahead () < AHEAD_US; n++) {
		bs_ticks++;
		music_callback ();
		// a tick of the PIT at this divider, in microseconds
		t = (uint64_t)music_divider * 1000000ull + music_rem;
		us = (uint32_t)(t / 1193182ull);
		music_rem = (uint32_t)(t - (uint64_t)us * 1193182ull);
		snd_wait ((int)us);
	}
}

// ---- the effects ----

enum { NVOICES = SND_CHANNELS };
typedef struct {
	int handle;			// 0: free
	int priority;
	uintptr_t callbackval;
	int loop;
	char *data;			// the samples, unsigned 8-bit mono
	int len, rate, vol;
	char *loopdata;			// where a looped sound plays from again
	int looplen;
	char *own;			// converted samples this voice allocated, or 0
	unsigned started;		// when it started, in sounds started: the oldest is the least
} fxvoice;
static fxvoice voice[NVOICES];
static int next_handle = 1;
static unsigned fx_started;
static int fx_volume = 255, fx_reverse;
static void (*fx_callback)(uintptr_t);

static void fx_end (int i)
{
	fxvoice *v = &voice[i];
	uintptr_t cb = v->callbackval;
	int was = v->handle;
	if (v->own)
		free (v->own);
	v->own = 0;
	v->handle = 0;
	if (was && fx_callback)
		fx_callback (cb);
}

// voices whose channel has gone quiet: looped ones play again from their
// loop, the rest are finished, and the game is told
static void fx_reap (void)
{
	int i, busy = snd_busy ();
	for (i = 0; i < NVOICES; i++)
		if (voice[i].handle && !(busy & (1 << i))) {
			if (voice[i].loop)
				snd_play (i, voice[i].loopdata, voice[i].looplen, voice[i].rate, voice[i].vol);
			else
				fx_end (i);
		}
}

static int fx_silent (int i) { return voice[i].vol == 0; }

// a channel for a sound of this priority: a free one; else, as audiolib's
// MV_AllocVoice chose, one of the lowest priority playing, if that is at
// most this sound's -- of those, one that cannot be heard, else the one
// that started first. -1 if there is none.
static int fx_channel (int priority)
{
	int i, low = 0 - 1;
	fx_reap ();
	for (i = 0; i < NVOICES; i++)
		if (!voice[i].handle)
			return i;
	for (i = 0; i < NVOICES; i++) {
		if (voice[i].priority > priority)
			continue;
		if (low < 0 || voice[i].priority < voice[low].priority) {
			low = i;
			continue;
		}
		if (voice[i].priority > voice[low].priority || fx_silent (low))
			continue;
		if (fx_silent (i) || (int)(voice[i].started - voice[low].started) < 0)
			low = i;
	}
	return low;
}

static int fx_find (int handle)
{
	int i;
	if (handle <= 0)
		return 0 - 1;
	for (i = 0; i < NVOICES; i++)
		if (voice[i].handle == handle)
			return i;
	return 0 - 1;
}

// left and right (0-255 each) as the card wants them, the volume and the
// reversal applied
static int fx_lr (int left, int right)
{
	int t;
	if (left < 0) left = 0;
	if (left > 255) left = 255;
	if (right < 0) right = 0;
	if (right > 255) right = 255;
	if (fx_reverse) { t = left; left = right; right = t; }
	left = left * fx_volume / 255;
	right = right * fx_volume / 255;
	return left | (right << 8);
}

// the rate moved by PITCHOFFSET cents, as audiolib's PITCH_GetScale did
static int fx_pitched (int rate, int cents)
{
	static int semitone[13] = {
		65536, 69433, 73562, 77936, 82570, 87480, 92682,
		98193, 104032, 110218, 116773, 123716, 131072 };
	int oct, n, s, scale;
	if (cents == 0)
		return rate;
	oct = cents / 1200;
	n = cents - oct * 1200;
	if (n < 0) { n += 1200; oct--; }
	s = n / 100;
	scale = semitone[s] + (semitone[s + 1] - semitone[s]) * (n % 100) / 100;
	rate = (rate >> 16) * scale + (((rate & 0xffff) * (scale >> 4)) >> 12);
	if (oct > 0) rate <<= oct;
	if (oct < 0) rate >>= -oct;
	return rate;
}

// audiolib's MV_Pan3D: 32 angles round the listener, 0 ahead and 8 to the
// right; the far ear's level falls to nothing over a quarter turn, and
// distance (0-255) takes the level down for both
static void fx_pan3d (int angle, int distance, int *left, int *right)
{
	int level, i, d;
	if (distance < 0) { distance = -distance; angle += 16; }
	if (distance > 255) distance = 255;
	level = 255 - distance;
	i = angle & 31;
	*left = *right = level;
	if (i <= 16) {
		d = i <= 8 ? i : 16 - i;
		*left = level - level * d / 8;
	} else {
		d = i <= 22 ? i - 16 : i <= 24 ? 8 : 31 - i;
		*right = level - level * d / 8;
	}
}

static int le16 (unsigned char *p) { return p[0] | (p[1] << 8); }
static int le32 (unsigned char *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }

// A Creative Voice File's sound data: type 1 blocks of unsigned 8-bit
// PCM, with their continuations (type 2), joined in memory of their own
// when there is more than one. Audiolib's VOC header gives no length, so
// the blocks are trusted to end with a terminator (type 0), as Blood's do.
static int voc_parse (unsigned char *p, char **data, int *len, int *rate, char **own)
{
	unsigned char *q, *body;
	char *join;
	int type, blen, total, n;
	*own = 0;
	if (memcmp (p, "Creative Voice File", 19) != 0)
		return 0;
	q = p + le16 (p + 20);
	total = 0; n = 0; *rate = 0; *data = 0;
	for (body = q; (type = body[0]) != 0 && type != 7; body = body + 4 + blen) {
		blen = body[1] | (body[2] << 8) | (body[3] << 16);
		if (type == 1) {
			if (body[5] != 0)
				return 0;		// only unsigned 8-bit PCM
			if (!*rate)
				*rate = 1000000 / (256 - body[4]);
			if (!n)
				*data = (char *)body + 6;
			total += blen - 2; n++;
		} else if (type == 2) {
			total += blen; n++;
		}
	}
	if (!n || !*rate || total <= 0)
		return 0;
	*len = total;
	if (n == 1)
		return 1;
	if (!(join = malloc (total)))
		return 0;
	total = 0;
	for (body = q; (type = body[0]) != 0 && type != 7; body = body + 4 + blen) {
		blen = body[1] | (body[2] << 8) | (body[3] << 16);
		if (type == 1) { memcpy (join + total, body + 6, blen - 2); total += blen - 2; }
		else if (type == 2) { memcpy (join + total, body + 4, blen); total += blen; }
	}
	*data = join; *own = join;
	return 1;
}

// A RIFF WAVE: 8-bit mono as it is; 16-bit, or stereo, made 8-bit mono in
// memory of its own
static int wav_parse (unsigned char *p, char **data, int *len, int *rate, char **own)
{
	unsigned char *q, *end, *pcm = 0;
	int n = 0, size, channels = 0, bits = 0, i, frames, v;
	char *out;
	*own = 0;
	if (memcmp (p, "RIFF", 4) != 0 || memcmp (p + 8, "WAVE", 4) != 0)
		return 0;
	end = p + 8 + le32 (p + 4);
	for (q = p + 12; q + 8 <= end; q = q + 8 + ((size + 1) & ~1)) {
		size = le32 (q + 4);
		if (memcmp (q, "fmt ", 4) == 0) {
			if (le16 (q + 8) != 1)
				return 0;		// PCM
			channels = le16 (q + 10);
			*rate = le32 (q + 12);
			bits = le16 (q + 22);
		} else if (memcmp (q, "data", 4) == 0) {
			pcm = q + 8; n = size;
			if (pcm + n > end)
				n = (int)(end - pcm);
		}
	}
	if (!pcm || !channels || (bits != 8 && bits != 16))
		return 0;
	if (bits == 8 && channels == 1) {
		*data = (char *)pcm; *len = n;
		return 1;
	}
	frames = n / (channels * (bits / 8));
	if (!(out = malloc (frames)))
		return 0;
	for (i = 0; i < frames; i++) {
		if (bits == 8)
			v = (pcm[i * channels] + pcm[i * channels + channels - 1]) / 2;
		else {
			v = (short)le16 (pcm + i * channels * 2) + (short)le16 (pcm + (i * channels + channels - 1) * 2);
			v = (v / 2 >> 8) + 128;
		}
		out[i] = (char)v;
	}
	*data = out; *len = frames; *own = out;
	return 1;
}

static int fx_start (char *data, int len, int rate, char *own, int pitchoffset, int left, int right,
                     int priority, uintptr_t callbackval, char *loopdata, int looplen)
{
	int i;
	fxvoice *v;
	if (!fx_on || len <= 0) {
		if (own) free (own);
		return FX_Warning;
	}
	if ((i = fx_channel (priority)) < 0) {
		if (own) free (own);
		return FX_Warning;
	}
	if (voice[i].handle) {
		snd_stop (i);
		fx_end (i);
	}
	v = &voice[i];
	v->handle = next_handle++;
	if (next_handle > 0x7fff)
		next_handle = 1;
	v->priority = priority;
	v->started = fx_started++;
	v->callbackval = callbackval;
	v->loop = loopdata != 0 && looplen > 0;
	v->loopdata = loopdata;
	v->looplen = looplen;
	v->data = data; v->len = len; v->own = own;
	v->rate = fx_pitched (rate, pitchoffset);
	v->vol = fx_lr (left, right);
	snd_play (i, v->data, v->len, v->rate, v->vol);
	return v->handle;
}

// a VOC or a WAV, by its header; looped, from its beginning again
static int fx_play (char *ptr, int pitchoffset, int left, int right, int priority,
                    uintptr_t callbackval, int loop)
{
	char *data, *own;
	int len, rate;
	if (!fx_on)
		return FX_Warning;
	if (!voc_parse ((unsigned char *)ptr, &data, &len, &rate, &own) &&
	    !wav_parse ((unsigned char *)ptr, &data, &len, &rate, &own)) {
		FX_ErrorCode = FX_InvalidCard;
		return FX_Error;
	}
	return fx_start (data, len, rate, own, pitchoffset, left, right, priority, callbackval,
	                 loop ? data : 0, loop ? len : 0);
}

char *FX_ErrorString (int ErrorNumber)
{
	if (ErrorNumber == FX_Warning || ErrorNumber == FX_Error)
		ErrorNumber = FX_ErrorCode;
	if (ErrorNumber == FX_Ok)
		return "Fx ok.";
	if (ErrorNumber == C4_NoCard)
		return "The machine has no sound card with sample channels.";
	return "Unknown Fx error code.";
}

// Blood stops on a card it cannot set up: every card is set up, and with
// none there (or none with sample channels) the sounds go nowhere
int FX_SetupCard (int SoundCard, fx_device *device)
{
	(void)SoundCard;
	device->MaxVoices = NVOICES;
	device->MaxSampleBits = 8;
	device->MaxChannels = 2;
	return FX_Ok;
}

int FX_GetBlasterSettings (fx_blaster_config *blaster)
{
	blaster->Address = 0x220;
	blaster->Type = 6;
	blaster->Interrupt = 5;
	blaster->Dma8 = 1;
	blaster->Dma16 = 5;
	blaster->Midi = 0x330;
	blaster->Emu = 0x620;
	return FX_Ok;
}

int FX_SetupSoundBlaster (fx_blaster_config blaster, int *MaxVoices, int *MaxSampleBits, int *MaxChannels)
{
	(void)blaster;
	*MaxVoices = NVOICES;
	*MaxSampleBits = 8;
	*MaxChannels = 2;
	return FX_Ok;
}

int FX_Init (int SoundCard, int numvoices, int numchannels, int samplebits, uint32_t mixrate)
{
	(void)SoundCard; (void)numvoices; (void)numchannels; (void)samplebits; (void)mixrate;
	if (fx_on)
		FX_Shutdown ();
	memset (voice, 0, sizeof voice);
	fx_on = card && (snd_caps () & SND_HAS_PCM);
	return FX_Ok;
}

int FX_Shutdown (void)
{
	if (fx_on)
		FX_StopAllSounds ();
	fx_on = 0;
	return FX_Ok;
}

int  FX_SetCallBack (void (*function)(uintptr_t)) { fx_callback = function; return FX_Ok; }
void FX_SetVolume (int volume) { fx_volume = volume < 0 ? 0 : volume > 255 ? 255 : volume; }
int  FX_GetVolume (void) { return fx_volume; }
void FX_SetReverseStereo (int setting) { fx_reverse = setting; }
int  FX_GetReverseStereo (void) { return fx_reverse; }
void FX_SetReverb (int reverb) { (void)reverb; }
void FX_SetFastReverb (int reverb) { (void)reverb; }
int  FX_GetMaxReverbDelay (void) { return 0; }
int  FX_GetReverbDelay (void) { return 0; }
void FX_SetReverbDelay (int delay) { (void)delay; }

int FX_VoiceAvailable (int priority)
{
	int i;
	if (!fx_on)
		return 0;
	fx_reap ();
	for (i = 0; i < NVOICES; i++)
		if (!voice[i].handle || voice[i].priority <= priority)
			return 1;
	return 0;
}

int FX_EndLooping (int handle)
{
	int i = fx_find (handle);
	if (i >= 0)
		voice[i].loop = 0;
	return FX_Ok;
}

int FX_SetPan (int handle, int vol, int left, int right)
{
	int i = fx_find (handle);
	(void)vol;
	if (i < 0)
		return FX_Warning;
	voice[i].vol = fx_lr (left, right);
	snd_volume (i, voice[i].vol);
	return FX_Ok;
}

// a sound's pitch is its rate, which the card takes when it starts
int FX_SetPitch (int handle, int pitchoffset) { (void)handle; (void)pitchoffset; return FX_Ok; }
int FX_SetFrequency (int handle, int frequency) { (void)handle; (void)frequency; return FX_Ok; }

int FX_PlayVOC (char *ptr, int pitchoffset, int vol, int left, int right, int priority, uintptr_t callbackval)
{
	(void)vol;
	return fx_play (ptr, pitchoffset, left, right, priority, callbackval, 0);
}

int FX_PlayLoopedVOC (char *ptr, int32_t loopstart, int32_t loopend, int pitchoffset, int vol, int left,
                      int right, int priority, uintptr_t callbackval)
{
	(void)loopstart; (void)loopend; (void)vol;
	return fx_play (ptr, pitchoffset, left, right, priority, callbackval, 1);
}

int FX_PlayWAV (char *ptr, int pitchoffset, int vol, int left, int right, int priority, uintptr_t callbackval)
{
	(void)vol;
	return fx_play (ptr, pitchoffset, left, right, priority, callbackval, 0);
}

int FX_PlayLoopedWAV (char *ptr, int32_t loopstart, int32_t loopend, int pitchoffset, int vol, int left,
                      int right, int priority, uintptr_t callbackval)
{
	(void)loopstart; (void)loopend; (void)vol;
	return fx_play (ptr, pitchoffset, left, right, priority, callbackval, 1);
}

int FX_PlayVOC3D (char *ptr, int pitchoffset, int angle, int distance, int priority, uintptr_t callbackval)
{
	int left, right;
	fx_pan3d (angle, distance, &left, &right);
	return fx_play (ptr, pitchoffset, left, right, priority, callbackval, 0);
}

int FX_PlayWAV3D (char *ptr, int pitchoffset, int angle, int distance, int priority, uintptr_t callbackval)
{
	return FX_PlayVOC3D (ptr, pitchoffset, angle, distance, priority, callbackval);
}

int FX_PlayRaw (char *ptr, uint32_t length, uint32_t rate, int pitchoffset, int vol, int left, int right,
                int priority, uintptr_t callbackval)
{
	(void)vol;
	return fx_start (ptr, (int)length, (int)rate, 0, pitchoffset, left, right, priority, callbackval, 0, 0);
}

// the whole sound first, then from LOOPSTART to LOOPEND again and again
int FX_PlayLoopedRaw (char *ptr, uint32_t length, char *loopstart, char *loopend, uint32_t rate,
                      int pitchoffset, int vol, int left, int right, int priority, uintptr_t callbackval)
{
	int n;
	(void)vol;
	if (!loopstart || loopstart < ptr || loopstart >= ptr + length)
		loopstart = ptr;
	if (!loopend || loopend <= loopstart || loopend > ptr + length)
		loopend = ptr + length;
	n = (int)(loopend - loopstart);
	return fx_start (ptr, (int)length, (int)rate, 0, pitchoffset, left, right, priority, callbackval, loopstart, n);
}

int FX_Pan3D (int handle, int angle, int distance)
{
	int i = fx_find (handle), left, right;
	if (i < 0)
		return FX_Warning;
	fx_pan3d (angle, distance, &left, &right);
	voice[i].vol = fx_lr (left, right);
	snd_volume (i, voice[i].vol);
	return FX_Ok;
}

int FX_SoundActive (int handle)
{
	int i = fx_find (handle);
	return i >= 0 && ((snd_busy () & (1 << i)) || voice[i].loop);
}

int FX_SoundsPlaying (void)
{
	int i, n = 0;
	if (!fx_on)
		return 0;
	fx_reap ();
	for (i = 0; i < NVOICES; i++)
		if (voice[i].handle)
			n++;
	return n;
}

int FX_StopSound (int handle)
{
	int i = fx_find (handle);
	if (i < 0)
		return FX_Warning;
	snd_stop (i);
	fx_end (i);
	return FX_Ok;
}

int FX_StopAllSounds (void)
{
	int i;
	if (!fx_on)
		return FX_Ok;
	snd_stop (0 - 1);
	for (i = 0; i < NVOICES; i++)
		if (voice[i].handle)
			fx_end (i);
	return FX_Ok;
}

int FX_StartDemandFeedPlayback (void (*function)(char **ptr, uint32_t *length), int rate, int pitchoffset,
                                int vol, int left, int right, int priority, uintptr_t callbackval)
{
	(void)function; (void)rate; (void)pitchoffset; (void)vol; (void)left; (void)right; (void)priority;
	(void)callbackval;
	return FX_Warning;
}

int FX_StartRecording (int MixRate, void (*function)(char *ptr, int length))
{
	(void)MixRate; (void)function;
	return FX_Warning;
}

void FX_StopRecord (void) {}
