// accel.mjs -- Blood's accelerators for c4's libjs (its libjs/accel.js,
// docs/accel.md): the engine's innermost loops, chocolate-blood's
// source/build/a_asm.c (Ken Silverman's A.ASM in C), run by the host when
// the machine has them. c4/inc/c4accel.h is the game's side, with each
// operation's number (BA_*); a_asm.c calls them under c4lc and keeps its
// own C for every other machine. c4pc has them too, in C (its
// src/c4pc/games/blood.c), each as this one.
//
// Each does exactly what a_asm.c's C does, which is what A.ASM did: the
// same pixels in the same order, and the same state after. So they keep
// what Shadow Warrior's and Duke's loops have no need of -- hlineasm4's
// coordinates as a 64-bit pair, stepped down with a borrow; vlineasm4's
// four columns packed into a 64-bit sum and a 16-bit one whose carry goes
// to another column; a texel put in its offset's low byte before the
// palette is read with the whole offset; mvlineasm4's counters in the low
// bytes of vplce[0] and of asm3. 32-bit arithmetic wrapping, and shifts
// modulo 32, as c4's SHL and LSR take them (JavaScript's own). Pixels are
// written as they are made, and vplce only at the end: a page that faults
// part of the way runs the instruction again from its start, which, reading
// nothing it writes, makes the same pixels again.
export default function blood({ reader, writer, reader32, writer32, args }) {
  // BA_HLINE4 (cnt, pofs, edx, esi, edi, asm1, asm2, sublo, subhi, al, bl,
  // tex, pal): hlineasm4, right to left from edi; pofs the shade's offset
  // in the palette (its ecx); sub hlineasm4_sub, the step of the 64-bit
  // pair; al and bl sethlinesizes', tex its texture, pal
  // setpalookupaddress's
  function hline4(m) {
    let [cnt, pofs, edx, esi, edi, a1, a2, sublo, subhi, al, bl, tex, pal] = args(m, 13);
    const rt = reader(m), rp = reader(m), wr = writer(m);
    const aln = (32 - al) & 255, bln = (32 - bl) & 255;     // uint8 in a_asm.c
    const mask = 0xffffffff >>> (32 - al - bl);
    // a pixel by the coordinates as two words (the first pixels', and a
    // short row's)
    const px2 = (s, d) => rp((pal + rt((tex + ((((s >>> aln) << bl) | (d >>> bln)) | 0)) | 0) + pofs) | 0);
    // ... and by an offset into the texture (the pair's rows)
    const px = a => rp((pal + rt((tex + a) | 0) + pofs) | 0);
    let ebp = (cnt + 1) | 0;
    if (ebp <= 8) {
      while (ebp) {
        wr(edi, px2(esi, edx));
        esi = (esi - a1) | 0; edx = (edx - a2) | 0;
        edi = (edi - 1) | 0; ebp = (ebp - 1) | 0;
      }
      return 0;
    }
    if ((edi & 1) === 0) {
      wr(edi, px2(esi, edx));
      esi = (esi - a1) | 0; edx = (edx - a2) | 0;
      edi = (edi - 1) | 0; ebp = (ebp - 1) | 0;
    }
    if ((edi & 2) === 0) {
      const bh = px2(esi, edx);
      esi = (esi - a1) | 0; edx = (edx - a2) | 0;
      const bl2 = px2(esi, edx);
      esi = (esi - a1) | 0; edx = (edx - a2) | 0;
      wr((edi - 1) | 0, bl2); wr(edi, bh);
      edi = (edi - 2) | 0; ebp = (ebp - 2) | 0;
    }
    // the pair: l low, h high
    let l = esi, h = edx;
    h = (h & ~255) | ((l >>> aln) & 255);
    l = l << al;
    edi = (edi + 1) | 0;
    // one texel's offset from h, then the pair less the step
    const step = () => {
      const a = ((h << bl) | (h >>> (32 - bl))) & mask;
      const lo = (l - sublo) | 0;
      h = (h - subhi - ((l >>> 0) < (sublo >>> 0) ? 1 : 0)) | 0;
      l = lo;
      return a;
    };
    for (;;) {
      let o = step();
      edi = (edi - 4) | 0;
      let d = px(o);
      o = step(); const c = px(o);
      o = step(); const b = px(o);
      o = step(); const a = px(o);
      ebp = (ebp - 4) | 0;
      if ((ebp >>> 0) < 0xfffffffc) {
        wr(edi, a); wr((edi + 1) | 0, b); wr((edi + 2) | 0, c); wr((edi + 3) | 0, d);
        continue;
      }
      if (ebp & 2) {
        wr((edi + 2) | 0, c); wr((edi + 3) | 0, d);
        d = b;
        edi = (edi - 2) | 0;
      }
      if (ebp & 1) wr((edi + 3) | 0, d);
      return 0;
    }
  }

  // the four columns' arrays: vplce, vince, bufplce, palookupoffse
  const four = (r32, p) => [r32(p), r32((p + 4) | 0), r32((p + 8) | 0), r32((p + 12) | 0)];

  // BA_VLINE4 (edi, ylk, pv, pi, pb, pp, al, aln, aln16, almask, bpl):
  // vlineasm4 from edi down ylk bytes (ylookup[ecx]), a row of four each
  // bpl; pv vplce, written back
  function vline4(m) {
    let [edi, ylk, pv, pi, pb, pp, al, aln, aln16, almask, bpl] = args(m, 11);
    const r32 = reader32(m), w32 = writer32(m), rt = reader(m), rp = reader(m), wr = writer(m);
    const vp = four(r32, pv), vi = four(r32, pi), buf = four(r32, pb), pal = four(r32, pp);
    const v1 = (edi + ylk) | 0;
    edi = (edi - v1) | 0;
    const rot = x => (x << aln) | (x >>> (32 - aln));
    let va = rot(vi[3]), vb = rot(vi[1]);
    const add1l = va & 0xffff0000, add1h = ((vi[2] & 0xfffffe00) + (va & 0x1ff)) | 0;
    const add2l = (vb >>> 16) & 0xffff, add2h = ((vi[0] & 0xfffffe00) + (vb & 0x1ff)) | 0;
    va = rot(vp[3]); vb = rot(vp[1]);
    let l = (va & 0xffff0000) | (vb >>> 16);
    let h = ((vp[2] & 0xfffffe00) + (va & 0x1ff)) | 0;
    let h2 = ((vp[0] & 0xfffffe00) + (vb & 0x1ff)) | 0;
    do {
      let o0 = h2 >>> al, o1 = h2 & almask, o2 = h >>> al, o3 = h & almask;
      // the 64-bit sum, then the 16-bit one, whose carry is h2's
      const nl = (l + add1l) | 0;
      h = (h + add1h + ((nl >>> 0) < (l >>> 0) ? 1 : 0)) | 0;
      l = nl;
      const lw = ((l & 0xffff) + add2l) & 0xffff;
      l = (l & 0xffff0000) | lw;
      if (lw < add2l) h2 = (h2 + 1) | 0;
      h2 = (h2 + add2h) | 0;
      // each texel in its offset's low byte, and the palette by that
      o0 = (o0 & ~255) | rt((buf[0] + o0) | 0);
      o1 = (o1 & ~255) | rt((buf[1] + o1) | 0);
      o2 = (o2 & ~255) | rt((buf[2] + o2) | 0);
      o3 = (o3 & ~255) | rt((buf[3] + o3) | 0);
      const at = (v1 + edi) | 0;
      wr(at, rp((pal[0] + o0) | 0));
      wr((at + 1) | 0, rp((pal[1] + o1) | 0));
      wr((at + 2) | 0, rp((pal[2] + o2) | 0));
      wr((at + 3) | 0, rp((pal[3] + o3) | 0));
      edi = (edi + bpl) | 0;
    } while (edi < 0);
    w32((pv + 8) | 0, h);
    w32(pv, h2);
    w32((pv + 12) | 0, ((h << al) + (l >>> aln)) | 0);
    w32((pv + 4) | 0, ((h2 << al) + ((l >>> aln16) & 0xffff)) | 0);
    return 0;
  }

  // BA_MVLINE4 (ecx, edi, pv, pi, pb, pp, al, bpl): mvlineasm4, the masked
  // four, texel 255 left out; its counter in vplce[0]'s low byte, and the
  // blocks of 256 in asm3's (left 0, which a_asm.c sees to)
  function mvline4(m) {
    let [ecx, edi, pv, pi, pb, pp, al, bpl] = args(m, 8);
    const r32 = reader32(m), w32 = writer32(m), rt = reader(m), rp = reader(m), wr = writer(m);
    const vi = four(r32, pi), buf = four(r32, pb), pal = four(r32, pp);
    let [p0, p1, p2, p3] = four(r32, pv);
    const i0 = vi[0] & ~255, i1 = vi[1] & ~255, i2 = vi[2], i3 = vi[3];
    p0 = (p0 & ~255) | ((ecx + 1) & 255);
    let blocks = ((ecx >> 8) + 1) & 255;
    edi = (edi - bpl) | 0;
    do {
      for (;;) {
        const cl = ((p0 & 255) - 1) & 255;
        p0 = (p0 & ~255) | cl;
        if (!cl) break;
        let o0 = p0 >>> al, o1 = p1 >>> al, o2 = p2 >>> al, o3 = p3 >>> al;
        const t0 = rt((buf[0] + o0) | 0), t1 = rt((buf[1] + o1) | 0);
        const t2 = rt((buf[2] + o2) | 0), t3 = rt((buf[3] + o3) | 0);
        o0 = (o0 & ~255) | t0; o1 = (o1 & ~255) | t1; o2 = (o2 & ~255) | t2; o3 = (o3 & ~255) | t3;
        p0 = (p0 + i0) | 0; p1 = (p1 + i1) | 0; p2 = (p2 + i2) | 0; p3 = (p3 + i3) | 0;
        const ca = rp((pal[0] + o0) | 0), cb = rp((pal[1] + o1) | 0);
        const cc = rp((pal[2] + o2) | 0), cd = rp((pal[3] + o3) | 0);
        edi = (edi + bpl) | 0;
        if (t0 < 255) wr(edi, ca);
        if (t1 < 255) wr((edi + 1) | 0, cb);
        if (t2 < 255) wr((edi + 2) | 0, cc);
        if (t3 < 255) wr((edi + 3) | 0, cd);
      }
      blocks = (blocks - 1) & 255;
    } while (blocks);
    w32(pv, p0); w32((pv + 4) | 0, p1); w32((pv + 8) | 0, p2); w32((pv + 12) | 0, p3);
    return 0;
  }

  // BA_VLINE1 (eax, ebx, ecx, edx, esi, edi, al, bpl): vlineasm1, ecx + 1
  // pixels down from edi; the coordinate after
  function vline1(m) {
    let [inc, pal, n, v, buf, edi, al, bpl] = args(m, 8);
    const rt = reader(m), rp = reader(m), wr = writer(m);
    n = (n + 1) | 0;
    edi = (edi - bpl) | 0;
    do {
      const a = v >>> al;
      v = (v + inc) | 0;
      edi = (edi + bpl) | 0;
      wr(edi, rp((pal + rt((buf + a) | 0)) | 0));
      n = (n - 1) | 0;
    } while (n);
    return v;
  }

  // BA_MVLINE1 (eax, ebx, ecx, edx, esi, edi, al, bpl): mvlineasm1, texel
  // 255 left out
  function mvline1(m) {
    let [inc, pal, n, v, buf, edi, al, bpl] = args(m, 8);
    const rt = reader(m), rp = reader(m), wr = writer(m);
    do {
      const a = v >>> al;
      v = (v + inc) | 0;
      const c = rt((buf + a) | 0);
      if (c !== 255) wr(edi, rp((pal + c) | 0));
      edi = (edi + bpl) | 0;
      n = (n - 1) | 0;
    } while (n >= 0);
    return v;
  }

  return [hline4, vline4, mvline4, vline1, mvline1];
}
