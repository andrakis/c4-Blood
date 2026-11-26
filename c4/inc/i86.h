// i86.h -- Watcom's DOS/DPMI register interface, which Blood-RE's game code
// names (CD audio, the timer, the joystick). There is no DOS under it here:
// the types are as Watcom had them, and int386 and int386x do nothing and
// return with the carry set, as a failed DOS call does.
#ifndef C4BLOOD_I86_H
#define C4BLOOD_I86_H 1
#ifdef __cplusplus
extern "C" {
#endif

struct DWORDREGS { unsigned int eax, ebx, ecx, edx, esi, edi, cflag; };
struct WORDREGS  { unsigned short ax, _1, bx, _2, cx, _3, dx, _4, si, _5, di, _6; unsigned int cflag; };
struct BYTEREGS  { unsigned char al, ah, _1[2], bl, bh, _2[2], cl, ch, _3[2], dl, dh, _4[2]; };
union REGS { struct DWORDREGS x; struct WORDREGS w; struct BYTEREGS h; };
struct SREGS { unsigned short es, cs, ss, ds, fs, gs; };

int int386 (int n, union REGS *in, union REGS *out);
int int386x (int n, union REGS *in, union REGS *out, struct SREGS *s);
void segread (struct SREGS *s);

#define FP_SEG(p) ((unsigned short)0)
#define MK_FP(s, o) ((void *)(o))
#ifndef _enable
#define _enable()
#define _disable()
#endif
#define inp(port) 0
#define outp(port, v) ((void)(v))
#define inpw(port) 0
#define outpw(port, v) ((void)(v))
#ifndef __far
#define __far
#define _far
#define far
#define __near
#define near
#define __interrupt
#define interrupt
#define __loadds
#define _loadds
#endif

#ifdef __cplusplus
}
#endif
#endif
