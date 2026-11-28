/* c4accel.h -- c4's port: Blood's accelerators, c4/accel.mjs (libjs) and
   c4pc's src/c4pc/games/blood.c, from the game's side (c4's
   libjs/guest/accel.h, docs/accel.md): the engine's innermost loops,
   chocolate-blood's a_asm.c, one instruction each. They keep A.ASM's ways
   that the C there keeps -- hlineasm4's coordinates as a 64-bit pair,
   vlineasm4's four columns sharing a 64-bit sum and a 16-bit carry,
   mvlineasm4's counters in the low bytes of vplce[0] and asm3, a texel
   put in an offset's low byte before the palette is read -- so the
   pixels are the C's, which are B0's. blood_draw is the module's first
   number: -1 until blood-c4.c asks (accel_find), 0 for none; elsewhere,
   in gcc's build and c4's headless one, a_asm.c's C runs. The numbers
   are the module's operations in its order. */
#ifndef C4BLOOD_ACCEL_H
#define C4BLOOD_ACCEL_H 1

#include <stdint.h>

enum { BA_HLINE4, BA_VLINE4, BA_MVLINE4, BA_VLINE1, BA_MVLINE1 };

#ifdef __cplusplus
extern "C" {
#endif
extern int blood_draw;
#ifdef __cplusplus
}
#endif

#if defined(__c4_inttypes__) && defined(C4BLOOD_C4)
#define BA_OP(op) (blood_draw + (op))
/* hlineasm4: a floor's row, right to left, from EDI; the step's 64 bits
   (hlineasm4_sub) as two words, low first; sethlinesizes' al, bl and
   texture, setpalookupaddress's palette */
#define blood_hline4(cnt, pofs, edx, esi, edi, a1, a2, sublo, subhi, al, bl, tex, pal) \
	__c4_opcode ((cnt), (pofs), (edx), (esi), (edi), (a1), (a2), (sublo), (subhi), (al), (bl), (tex), (pal), \
		BA_OP (BA_HLINE4))
/* vlineasm4: four columns from EDI to EDI + YLK; vplce, vince, bufplce and
   palookupoffse by their addresses (vplce written back); setupvlineasm's
   al, al_n, al_n16 and al_mask; setvlinebpl's */
#define blood_vline4(edi, ylk, pv, pi, pb, pp, al, aln, aln16, almask, bpl) \
	__c4_opcode ((edi), (ylk), (pv), (pi), (pb), (pp), (al), (aln), (aln16), (almask), (bpl), BA_OP (BA_VLINE4))
/* mvlineasm4: the masked four; setupmvlineasm's al (asm3's low byte is
   left 0, as the loop leaves it: a_asm.c does that) */
#define blood_mvline4(ecx, edi, pv, pi, pb, pp, al, bpl) \
	__c4_opcode ((ecx), (edi), (pv), (pi), (pb), (pp), (al), (bpl), BA_OP (BA_MVLINE4))
/* vlineasm1 and mvlineasm1: one column; what they return, the coordinate
   after it */
#define blood_vline1(eax, ebx, ecx, edx, esi, edi, al, bpl) \
	__c4_opcode ((eax), (ebx), (ecx), (edx), (esi), (edi), (al), (bpl), BA_OP (BA_VLINE1))
#define blood_mvline1(eax, ebx, ecx, edx, esi, edi, al, bpl) \
	__c4_opcode ((eax), (ebx), (ecx), (edx), (esi), (edi), (al), (bpl), BA_OP (BA_MVLINE1))
#endif

#endif
