// acceltest.c -- c4's port: Shadow Warrior's accelerators (c4/accel.mjs)
// against the loops they stand for, each as JFBuild's a-c.c and pragmas.h
// have it: the same pixels, on made-up textures, palettes, steps and
// shifts, and the same values for any operands. test-sw runs it on c4's
// libjs. As Duke Nukem 3D's c4/acceltest.c.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "c4accel.h"

int32_t accel_sw = -1;
int32_t accel_sw_base (void) { return accel_sw = accel_find ("sw"); }

unsigned seed = 12345;
int rnd (int n) { seed = seed * 1103515245 + 12345; return (int)((seed >> 8) % (unsigned)n); }
int rnd32 (void) { return (rnd (65536) << 16) | rnd (65536); }

unsigned char *tex, *pal, *trans, *bufc, *bufa;
int recip[2048];
int fails, cases;
enum { TEXN = 1 << 20, PALN = 65536 + 512, TRANSN = 65536, BUFN = 320 * 240 };

void fill (unsigned char *p, int n, int holes)
{
	int i;
	for (i = 0; i < n; i++) { p[i] = rnd (256); if (holes && rnd (8) == 0) p[i] = 255; }
}
void check (char *what, int k, int same)
{
	cases++;
	if (memcmp (bufc, bufa, BUFN) != 0 || !same)
	{
		fails++;
		if (fails < 10) printf ("accel: %s, case %d, differs\n", what, k);
	}
}
void clear (void)
{
	int i;
	for (i = 0; i < BUFN; i++) bufc[i] = bufa[i] = rnd (256);
	memcpy (bufa, bufc, BUFN);
}

// a-c.c's loops, as they are there
void c_vline (int vinc, unsigned char *gpal, int cnt, unsigned int vplc, unsigned char *gbuf, unsigned char *pp,
	      int glogy, int bpl, int kind, unsigned char *gtrans, int transmode)
{
	unsigned char ch;
	for (; cnt >= 0; cnt--)
	{
		ch = gbuf[vplc >> glogy];
		if (kind == 0) *pp = gpal[ch];
		else if (ch != 255)
		{
			if (kind == 1) *pp = gpal[ch];
			else if (transmode) *pp = gtrans[(*pp) + (gpal[ch] << 8)];
			else *pp = gtrans[((*pp) << 8) + gpal[ch]];
		}
		pp += bpl;
		vplc += vinc;
	}
}
void c_hline (int cnt, unsigned char *palptr, unsigned int by, unsigned int bx, unsigned char *gbuf,
	      unsigned char *pp, int glogx, int glogy, int gbxinc, int gbyinc)
{
	for (; cnt >= 0; cnt--)
	{
		*pp = palptr[gbuf[((bx >> (32 - glogx)) << glogy) + (by >> (32 - glogy))]];
		bx -= gbxinc;
		by -= gbyinc;
		pp--;
	}
}
void c_mhline (unsigned char *gbuf, unsigned int bx, int cnt, unsigned int by, unsigned char *pp, int glogx,
	       int glogy, unsigned char *gpal, int asm1, int asm2, int t, unsigned char *gtrans, int transmode)
{
	unsigned char ch;
	for (; cnt > 0; cnt--)
	{
		ch = gbuf[((bx >> (32 - glogx)) << glogy) + (by >> (32 - glogy))];
		if (ch != 255)
		{
			if (!t) *pp = gpal[ch];
			else if (transmode) *pp = gtrans[(*pp) + (gpal[ch] << 8)];
			else *pp = gtrans[((*pp) << 8) + gpal[ch]];
		}
		bx += asm1;
		by += asm2;
		pp++;
	}
}
void c_sprv (int kind, int bx, int by, int cnt, unsigned char *gbuf, unsigned char *pp, unsigned char *gpal,
	     int gbxinc, int gbyinc, int glogy, int bpl, unsigned char *gtrans, int transmode)
{
	unsigned char ch;
	for (; cnt > 1; cnt--)
	{
		ch = gbuf[(bx >> 16) * glogy + (by >> 16)];
		if (kind == 0) *pp = gpal[ch];
		else if (ch != 255)
		{
			if (kind == 1) *pp = gpal[ch];
			else if (transmode) *pp = gtrans[(*pp) + (gpal[ch] << 8)];
			else *pp = gtrans[((*pp) << 8) + gpal[ch]];
		}
		bx += gbxinc;
		by += gbyinc;
		pp += bpl;
	}
}
int c_krecip (int i)
{
	float f = (float)i; i = *(int *)&f;
	return ((recip[(i >> 12) & 2047] >> (((i - 0x3f800000) >> 23) & 31)) ^ (i >> 31));
}
void c_slope (unsigned char *pp, intptr_t *slopalptr, int cnt, int bx, int by, int bz, int bzinc, int gx3,
	      int gy3, unsigned char *gbuf, int glogx, int glogy, int gpinc)
{
	int i;
	unsigned int u, v;
	for (; cnt > 0; cnt--)
	{
		i = c_krecip (bz >> 6); bz += bzinc;
		u = bx + gx3 * i;
		v = by + gy3 * i;
		*pp = *(unsigned char *)(slopalptr[0] + gbuf[((u >> (32 - glogx)) << glogy) + (v >> (32 - glogy))]);
		slopalptr--;
		pp += gpinc;
	}
}

// the texture's size, and a step that keeps an index within it
int logy_of (void) { return 1 + rnd (10); }

int main (void)
{
	int k, n, x, y, logx, logy, kind, mode, a, b, c, d, e, f, s, i, same;
	unsigned bx, by;
	int bxinc, byinc, bz, bzinc;
	intptr_t *slop;
	if (SW_ACCEL () <= 0)
	{
		printf ("accel: no accelerators here\n");
		return 1;
	}
	tex = malloc (TEXN); pal = malloc (PALN); trans = malloc (TRANSN);
	bufc = malloc (BUFN); bufa = malloc (BUFN); slop = malloc (512 * sizeof (intptr_t));
	fill (tex, TEXN, 1); fill (pal, PALN, 0); fill (trans, TRANSN, 0);
	for (i = 0; i < 2048; i++) recip[i] = rnd32 ();
	for (i = 0; i < 512; i++) slop[i] = (intptr_t)(pal + rnd (65536));

	// the columns: vline, mvline, tvline, each mode of the blend
	for (k = 0; k < 300; k++)
	{
		unsigned v; int inc; unsigned char *pp, *tx;
		kind = k % 3; mode = rnd (2);
		clear ();
		logy = 32 - logy_of ();
		n = rnd (200); x = rnd (320); y = rnd (20);
		v = (unsigned)rnd32 (); inc = rnd32 (); tx = tex + rnd (4096); pp = pal + rnd (256);
		c_vline (inc, pp, n, v, tx, bufc + y * 320 + x, logy, 320, kind, trans, mode);
		if (kind == 0) sw_vline (inc, pp, n, v, tx, bufa + y * 320 + x, logy, 320);
		else if (kind == 1) sw_mvline (inc, pp, n, v, tx, bufa + y * 320 + x, logy, 320);
		else sw_tvline (inc, pp, n, v, tx, bufa + y * 320 + x, logy, 320, trans, mode);
		check (kind == 0 ? "vline" : kind == 1 ? "mvline" : "tvline", k, 1);
	}
	// the floors' rows: hline right to left, mhline and thline left to right
	for (k = 0; k < 300; k++)
	{
		unsigned char *pp, *tx;
		kind = k % 3; mode = rnd (2);
		clear ();
		logx = 1 + rnd (9); logy = 1 + rnd (9);
		n = rnd (300); y = rnd (200);
		bx = (unsigned)rnd32 (); by = (unsigned)rnd32 (); bxinc = rnd32 (); byinc = rnd32 ();
		tx = tex + rnd (4096); pp = pal + rnd (256);
		if (kind == 0)
		{
			/* right to left from x = n, to x = 0 */
			c_hline (n, pp, by, bx, tx, bufc + y * 320 + n, logx, logy, bxinc, byinc);
			sw_hline (n, pp, by, bx, tx, bufa + y * 320 + n, logx, logy, bxinc, byinc);
			check ("hline", k, 1);
		}
		else
		{
			x = rnd (20);
			c_mhline (tx, bx, n, by, bufc + y * 320 + x, logx, logy, pp, bxinc, byinc, kind == 2, trans, mode);
			if (kind == 1) sw_mhline (tx, bx, n, by, bufa + y * 320 + x, logx, logy, pp, bxinc, byinc);
			else sw_thline (tx, bx, n, by, bufa + y * 320 + x, logx, logy, pp, bxinc, byinc, trans, mode);
			check (kind == 1 ? "mhline" : "thline", k, 1);
		}
	}
	// the sprites' columns: spritevline, mspritevline, tspritevline
	for (k = 0; k < 300; k++)
	{
		int sbx, sby, ysiz; unsigned char *pp;
		kind = k % 3; mode = rnd (2);
		clear ();
		ysiz = 1 + rnd (64);
		n = rnd (200); x = rnd (320); y = rnd (20);
		sbx = rnd (64) << 16 | rnd (65536); sby = rnd (ysiz) << 16 | rnd (65536);
		bxinc = rnd (65536) - 32768; byinc = rnd (65536);
		pp = pal + rnd (256);
		c_sprv (kind, sbx, sby, n, tex + 65536, bufc + y * 320 + x, pp, bxinc, byinc, ysiz, 320, trans, mode);
		sw_sprv (SA_SPRV + kind, sbx, sby, n, tex + 65536, bufa + y * 320 + x, pp, bxinc, byinc, ysiz, 320, trans, mode);
		check (kind == 0 ? "spritevline" : kind == 1 ? "mspritevline" : "tspritevline", k, 1);
	}
	// the sloped floors: slopevlin, its reciprocals from the table by a
	// float's bits
	for (k = 0; k < 200; k++)
	{
		int gx3, gy3, pinc; unsigned char *tx;
		clear ();
		logx = 1 + rnd (9); logy = 1 + rnd (9);
		n = rnd (200); x = rnd (320);
		bx = rnd32 (); by = rnd32 (); bz = rnd32 (); bzinc = rnd32 () >> rnd (16);
		gx3 = rnd32 () >> rnd (20); gy3 = rnd32 () >> rnd (20);
		pinc = rnd (2) ? 320 : -320;
		tx = tex + rnd (4096);
		c_slope (bufc + (pinc > 0 ? 0 : 239 * 320) + x, slop + 300, n, bx, by, bz, bzinc, gx3, gy3, tx, logx, logy, pinc);
		sw_slope (bufa + (pinc > 0 ? 0 : 239 * 320) + x, slop + 300, n, bx, by, bz, bzinc, gx3, gy3, tx, logx, logy,
			  pinc, recip);
		check ("slope", k, 1);
	}
	// the 64-bit helpers, against the int64_t code
	for (k = 0; k < 4000; k++)
	{
		a = rnd32 () >> rnd (32); b = rnd32 () >> rnd (32); c = rnd32 () >> rnd (32);
		d = rnd32 () >> rnd (32); e = rnd32 () >> rnd (32); f = rnd32 () >> rnd (32);
		if (k < 8) { a = k & 1 ? (int)0x80000000 : 0x7fffffff; b = k & 2 ? (int)0x80000000 : -1; }
		s = rnd (33);
		clear ();
		same = sw_mulscale (a, b, s) == (int)(((int64_t)a * b) >> s);
		check ("mulscale", k, same);
		same = sw_dmulscale (a, b, c, d, s) == (int)(((int64_t)a * b + (int64_t)c * d) >> s);
		check ("dmulscale", k, same);
		same = sw_tmulscale (a, b, c, d, e, f, s) == (int)(((int64_t)a * b + (int64_t)c * d + (int64_t)e * f) >> s);
		check ("tmulscale", k, same);
		if (c != 0 && !(c == -1 && a == (int)0x80000000))
		{
			same = sw_scale (a, b, c) == (int)(((int64_t)a * b) / c);
			check ("scale", k, same);
		}
		if (b != 0 && !(b == -1))
		{
			same = sw_divscale (a, b, s) == (int)(((int64_t)a << s) / b);
			check ("divscale", k, same);
		}
	}
	if (fails)
		printf ("accel: Shadow Warrior's, %d of %d cases differ from their loops\n", fails, cases);
	else
		printf ("accel: Shadow Warrior's, %d cases, each as its loop has it\n", cases);
	return fails != 0;
}
