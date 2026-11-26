// SDL3/SDL.h -- the SDL that chocolate-blood's game, engine and audiolib units
// use outside its platform files (build/system.c, video.c, sound.c, which
// c4blood replaces): the two clocks as atomic ints, the sound mutex, and a
// delay. One thread here, so the mutex does nothing. Reading a clock calls
// c4blood_clock_read() first, which is where the platform advances time and
// runs the timer interrupt (the PIT) that the SDL thread used to run.
#ifndef C4BLOOD_SDL_H
#define C4BLOOD_SDL_H 1
#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_AtomicInt { int value; } SDL_AtomicInt;
typedef struct SDL_Mutex SDL_Mutex;

void c4blood_clock_read (void);
void c4blood_delay (int ms);

static int SDL_GetAtomicInt (SDL_AtomicInt *a)
{
	c4blood_clock_read ();
	return a->value;
}
static int SDL_SetAtomicInt (SDL_AtomicInt *a, int v)
{
	int old = a->value;
	a->value = v;
	return old;
}
static int SDL_AddAtomicInt (SDL_AtomicInt *a, int v)
{
	int old = a->value;
	a->value = old + v;
	return old;
}
#define SDL_CreateMutex()    ((SDL_Mutex *)1)
#define SDL_LockMutex(m)     ((void)0)
#define SDL_UnlockMutex(m)   ((void)0)
#define SDL_Delay(ms)        c4blood_delay (ms)

#ifdef __cplusplus
}
#endif
#endif
