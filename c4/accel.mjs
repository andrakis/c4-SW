// accel.mjs -- Shadow Warrior's accelerators for c4's libjs (its libjs/
// accel.js, docs/accel.md): JFBuild's innermost loops (jfbuild/src/a-c.c)
// and its 64-bit helpers (pragmas.h, c4/sw-math.c), run by the host when
// the machine has them. c4/c4accel.h is the game's side, with each
// operation's number (SA_*); a-c.c and pragmas.h call them under c4lc,
// and keep their own C for every other machine. Each does exactly what
// that C does: the same pixels in the same order, 32-bit arithmetic
// wrapping, shifts modulo 32 as c4's SHL and LSR take them, and floats as
// c4's FPU makes them. As Duke Nukem 3D's (games/duke3d/c4/accel.mjs).
export default function sw({ reader, writer, reader32, args, fval, fbits }) {
  // SA_VLINE, SA_MVLINE (vinc, pal, cnt, vplc, buf, p, logy, bpl):
  // vlineasm1, mvlineasm1: for (; cnt >= 0; cnt--) { *p = pal[buf[vplc >>
  // logy]]; p += bpl; vplc += vinc; }, the masked one leaving texel 255 out
  function vline(m, masked) {
    let [inc, pal, n, v, buf, p, logy, bpl] = args(m, 8);
    const rt = reader(m), rp = reader(m), wr = writer(m);
    for (; n >= 0; n--, p = (p + bpl) | 0, v = (v + inc) | 0) {
      const c = rt((buf + (v >>> logy)) | 0);
      if (!masked || c !== 255) wr(p, rp((pal + c) | 0));
    }
    return 0;
  }

  // the translucent blend: transmode 1 is trans[*p + (c << 8)], 0 is
  // trans[(*p << 8) + c], c the texel's colour
  const blend = (tr, d, c, trans, mode) =>
    tr(mode ? (trans + d + (c << 8)) | 0 : (trans + (d << 8) + c) | 0);

  // SA_TVLINE (vinc, pal, cnt, vplc, buf, p, logy, bpl, trans, transmode):
  // tvlineasm1
  function tvline(m) {
    let [inc, pal, n, v, buf, p, logy, bpl, trans, mode] = args(m, 10);
    const rt = reader(m), rp = reader(m), rd = reader(m), rtr = reader(m), wr = writer(m);
    for (; n >= 0; n--, p = (p + bpl) | 0, v = (v + inc) | 0) {
      const c = rt((buf + (v >>> logy)) | 0);
      if (c !== 255) wr(p, blend(rtr, rd(p), rp((pal + c) | 0), trans, mode));
    }
    return 0;
  }

  // SA_HLINE (cnt, pal, by, bx, buf, p, logx, logy, bxinc, byinc):
  // hlineasm4, right to left: for (; cnt >= 0; cnt--) { *p-- =
  // pal[buf[((bx >> (32-logx)) << logy) + (by >> (32-logy))]]; bx -=
  // bxinc; by -= byinc; }
  function hline(m) {
    let [n, pal, by, bx, buf, p, logx, logy, bxinc, byinc] = args(m, 10);
    const sx = (32 - logx) & 31, sy = (32 - logy) & 31, ly = logy & 31;
    const rt = reader(m), rp = reader(m), wr = writer(m);
    for (; n >= 0; n--, p = (p - 1) | 0, bx = (bx - bxinc) | 0, by = (by - byinc) | 0)
      wr(p, rp((pal + rt((buf + (((bx >>> sx) << ly) + (by >>> sy))) | 0)) | 0));
    return 0;
  }

  // SA_MHLINE, SA_THLINE (buf, bx, cnt, by, p, logx, logy, pal, asm1, asm2,
  // trans, transmode): mhline, thline, left to right, cnt already >> 16
  function mhline(m, trans_) {
    let [buf, bx, n, by, p, logx, logy, pal, ax, ay, trans, mode] = args(m, 12);
    const sx = (32 - logx) & 31, sy = (32 - logy) & 31, ly = logy & 31;
    const rt = reader(m), rp = reader(m), rd = reader(m), rtr = reader(m), wr = writer(m);
    for (; n > 0; n--, p = (p + 1) | 0, bx = (bx + ax) | 0, by = (by + ay) | 0) {
      const c = rt((buf + (((bx >>> sx) << ly) + (by >>> sy))) | 0);
      if (c === 255) continue;
      wr(p, trans_ ? blend(rtr, rd(p), rp((pal + c) | 0), trans, mode) : rp((pal + c) | 0));
    }
    return 0;
  }

  // SA_SPRV, SA_MSPRV, SA_TSPRV (bx, by, cnt, buf, p, pal, bxinc, byinc,
  // ysiz, bpl, trans, transmode): spritevline, mspritevline, tspritevline:
  // for (; cnt > 1; cnt--) { c = buf[(bx >> 16) * ysiz + (by >> 16)]; ...;
  // bx += bxinc; by += byinc; p += bpl; }
  function sprv(m, kind) {
    let [bx, by, n, buf, p, pal, bxinc, byinc, ysiz, bpl, trans, mode] = args(m, 12);
    const rt = reader(m), rp = reader(m), rd = reader(m), rtr = reader(m), wr = writer(m);
    for (; n > 1; n--, bx = (bx + bxinc) | 0, by = (by + byinc) | 0, p = (p + bpl) | 0) {
      const c = rt((buf + Math.imul(bx >> 16, ysiz) + (by >> 16)) | 0);
      if (kind === 0) wr(p, rp((pal + c) | 0));
      else if (c !== 255) wr(p, kind === 1 ? rp((pal + c) | 0) : blend(rtr, rd(p), rp((pal + c) | 0), trans, mode));
    }
    return 0;
  }

  // krecipasm (engine.c): the reciprocal of i from the table by its float's
  // bits, (float)i as c4's FPU rounds it
  const krecip = (r32, recip, i) => {
    const u = fbits(Math.fround(i));
    return (r32((recip + 4 * ((u >> 12) & 2047)) | 0) >> (((u - 0x3f800000) >> 23) & 31)) ^ (u >> 31);
  };

  // SA_SLOPE (p, slopal, cnt, bx, by, bz, bzinc, gx3, gy3, buf, logx, logy,
  // pinc, recip): slopevlin, for (; cnt > 0; cnt--) { i = krecip(bz >> 6);
  // bz += bzinc; u = bx + gx3*i; v = by + gy3*i; *p = *(slopal[0] +
  // buf[((u >> (32-logx)) << logy) + (v >> (32-logy))]); slopal--; p +=
  // pinc; }, slopal a column of words
  function slope(m) {
    let [p, sp, n, bx, by, bz, bzinc, gx3, gy3, buf, logx, logy, pinc, recip] = args(m, 14);
    const sx = (32 - logx) & 31, sy = (32 - logy) & 31, ly = logy & 31;
    const rt = reader(m), rp = reader(m), r32 = reader32(m), rr = reader32(m), wr = writer(m);
    for (; n > 0; n--, sp = (sp - 4) | 0, p = (p + pinc) | 0) {
      const i = krecip(rr, recip, bz >> 6);
      bz = (bz + bzinc) | 0;
      const u = (bx + Math.imul(gx3, i)) | 0, v = (by + Math.imul(gy3, i)) | 0;
      wr(p, rp((r32(sp) + rt((buf + (((u >>> sx) << ly) + (v >>> sy))) | 0)) | 0));
    }
    return 0;
  }

  // the 64-bit helpers, exactly as int64_t: (int64_t)a * b as its two
  // words, hi signed and lo unsigned, in 16-bit digits (Duke's mul64)
  const mul64 = (a, b) => {
    const ah = a >> 16, al = a & 0xffff, bh = b >> 16, bl = b & 0xffff;
    const m1 = ah * bl, m2 = al * bh;
    let lo = (al * bl) >>> 0;
    let hi = (ah * bh + (m1 >> 16) + (m2 >> 16)) | 0;
    const t = (lo + ((m1 << 16) >>> 0)) >>> 0;
    if (t < lo) hi = (hi + 1) | 0;
    lo = (t + ((m2 << 16) >>> 0)) >>> 0;
    if (lo < t) hi = (hi + 1) | 0;
    return [hi, lo];
  };
  // hi:lo >> s, 0 <= s <= 32, arithmetically, its low word
  const shr64 = (hi, lo, s) => s === 0 ? lo | 0 : s >= 32 ? hi >> (s - 32) : ((lo >>> s) | (hi << (32 - s))) | 0;
  const add64 = ([h1, l1], [h2, l2]) => {
    const lo = (l1 + l2) >>> 0;
    return [(h1 + h2 + (lo < l1 ? 1 : 0)) | 0, lo];
  };
  // SA_MULSCALE (a, b, s), 0 <= s <= 32
  function mulscale(m) {
    const [a, b, s] = args(m, 3);
    const [hi, lo] = mul64(a, b);
    return shr64(hi, lo, s);
  }
  // SA_DMULSCALE (a, b, c, d, s): (a*b + c*d) >> s
  function dmulscale(m) {
    const [a, b, c, d, s] = args(m, 5);
    const [hi, lo] = add64(mul64(a, b), mul64(c, d));
    return shr64(hi, lo, s);
  }
  // SA_TMULSCALE (a, b, c, d, e, f, s): (a*b + c*d + e*f) >> s
  function tmulscale(m) {
    const [a, b, c, d, e, f, s] = args(m, 7);
    const [hi, lo] = add64(add64(mul64(a, b), mul64(c, d)), mul64(e, f));
    return shr64(hi, lo, s);
  }
  // SA_SCALE (a, b, c), c not 0: (int32_t)((int64_t)a * b / c), toward 0
  function scale(m) {
    const [a, b, c] = args(m, 3);
    return Number(BigInt.asIntN(32, (BigInt(a) * BigInt(b)) / BigInt(c)));
  }
  // SA_DIVSCALE (a, b, s), b not 0, 0 <= s <= 32: (int32_t)(((int64_t)a << s) / b)
  function divscale(m) {
    const [a, b, s] = args(m, 3);
    return Number(BigInt.asIntN(32, BigInt.asIntN(64, BigInt(a) << BigInt(s)) / BigInt(b)));
  }

  return [
    m => vline(m, false), m => vline(m, true), tvline, hline,
    m => mhline(m, false), m => mhline(m, true),
    m => sprv(m, 0), m => sprv(m, 1), m => sprv(m, 2),
    slope, mulscale, dmulscale, tmulscale, scale, divscale,
  ];
}
