// sw-math.c -- c4's port: JFBuild's 64-bit products and quotients
// (pragmas.h's mulscale, divscale, scale, dmulscale, tmulscale) from
// 32-bit operations, exactly the values the int64_t code gives.
//
// c4's machine has no 64-bit arithmetic: c4lc makes each operation on an
// int64_t a call, and a division a loop of them, a bit at a time. In SW's
// first build these were a quarter of a frame (__c4ll_mul 14.6%,
// __c4ll_udivmod 5.3%, __c4ll_sar 4.0% of a median one). pragmas.h tries
// the 32-bit ways first, where the product fits; these are the rest. From
// Duke Nukem 3D's port (c4's games/duke3d, Engine/src/fixedPoint_math.c),
// which found them; gcc's build keeps the int64_t code, and test-sw holds
// these to it, frame for frame.
#include <stdint.h>
#include "sw-math.h"
#include "c4accel.h"

// the first of Shadow Warrior's accelerators' numbers (c4/accel.mjs), 0
// for none, -1 until asked (SW_ACCEL)
int32_t accel_sw = -1;
int32_t accel_sw_base (void) { return accel_sw = accel_find ("sw"); }

/* x >> s, logically, for 0 < s < 32 */
#define C4_LSR(x, s) ((uint32_t)(x) >> (s))

/* how far b (0 < b < 2^30) shifts left and stays below 2^31 */
static int c4_room (int32_t b)
{
	int k = 0;
	if (b < 0x8000) { b <<= 16; k += 16; }
	if (b < 0x800000) { b <<= 8; k += 8; }
	if (b < 0x8000000) { b <<= 4; k += 4; }
	if (b < 0x20000000) { b <<= 2; k += 2; }
	if (b < 0x40000000) { b <<= 1; k += 1; }
	return k;
}

/* hi:lo / b for 0 < b < 2^30: the quotient's low 32 bits. Long division,
   as many bits at a time as the remainder has room for beside b. */
static uint32_t c4_udiv64 (uint32_t hi, uint32_t lo, int32_t b)
{
	int k, n = 64, take;
	int32_t r = 0;
	uint32_t q = 0;
	k = c4_room (b);
	if (hi == 0) { hi = lo; lo = 0; n = 32; }
	while (n > 0)
	{
		take = n < k ? n : k;
		r = (r << take) | (int32_t)C4_LSR (hi, 32 - take);
		hi = (hi << take) | C4_LSR (lo, 32 - take);
		lo <<= take;
		q = (q << take) | (uint32_t)(r / b);
		r = r % b;
		n -= take;
	}
	return q;
}

/* (int64_t)a * b as its two words, hi signed and lo unsigned: 16-bit
   digits, the high ones signed, so that any a and b take, INT32_MIN too */
static void c4_mul64 (int32_t a, int32_t b, int32_t *phi, uint32_t *plo)
{
	int32_t ah, bh, m1, m2, hi;
	uint32_t al, bl, lo, t;
	ah = a >> 16; al = a & 0xffff;
	bh = b >> 16; bl = b & 0xffff;
	m1 = ah * (int32_t)bl;			/* each within 2^31 */
	m2 = (int32_t)al * bh;
	lo = al * bl;				/* below 2^32 */
	hi = ah * bh + (m1 >> 16) + (m2 >> 16);
	t = lo + ((uint32_t)m1 << 16); hi += t < lo;
	lo = t + ((uint32_t)m2 << 16); hi += lo < t;
	*phi = hi;
	*plo = lo;
}

/* hi:lo >> s, 0 <= s <= 32, arithmetically, its low word */
static int32_t c4_shr64 (int32_t hi, uint32_t lo, int s)
{
	if (s == 0)
		return (int32_t)lo;
	if (s >= 32)
		return hi >> (s - 32);
	return (int32_t)((lo >> s) | ((uint32_t)hi << (32 - s)));
}

/* |a| * |b|, a and b > INT32_MIN, as hi:lo */
static void c4_umul64 (int32_t a, int32_t b, uint32_t *hi, uint32_t *lo)
{
	int32_t h;
	if (a < 0) a = -a;
	if (b < 0) b = -b;
	c4_mul64 (a, b, &h, lo);
	*hi = (uint32_t)h;
}

int32_t c4_mulscale (int32_t a, int32_t b, int32_t s)
{
	int32_t hi;
	uint32_t lo;
	c4_mul64 (a, b, &hi, &lo);
	return c4_shr64 (hi, lo, s);
}

/* (a*b + c*d) >> s */
int32_t c4_dmulscale (int32_t a, int32_t b, int32_t c, int32_t d, int32_t s)
{
	int32_t h1, h2;
	uint32_t l1, l2, lo;
	c4_mul64 (a, b, &h1, &l1);
	c4_mul64 (c, d, &h2, &l2);
	lo = l1 + l2;
	return c4_shr64 (h1 + h2 + (lo < l1), lo, s);
}

/* (a*b + c*d + e*f) >> s */
int32_t c4_tmulscale (int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f, int32_t s)
{
	int32_t h1, h2, h3, hi;
	uint32_t l1, l2, l3, lo;
	c4_mul64 (a, b, &h1, &l1);
	c4_mul64 (c, d, &h2, &l2);
	c4_mul64 (e, f, &h3, &l3);
	lo = l1 + l2;
	hi = h1 + h2 + (lo < l1);
	l1 = lo;
	lo = l1 + l3;
	hi += h3 + (lo < l1);
	return c4_shr64 (hi, lo, s);
}

/* (int32_t)(((int64_t)a << s) / d), for 0 <= s <= 32; *done 0 where these
   cannot (d at the ends of the range, a INT32_MIN) */
int32_t c4_divscale (int32_t a, int32_t d, int32_t s, int *done)
{
	uint32_t m, hi, lo, q;
	*done = 0;
	if (d == 0 || d >= 0x40000000 || d <= -0x40000000 || a == INT32_MIN || s < 0 || s > 32)
		return 0;
	m = a < 0 ? (uint32_t)-a : (uint32_t)a;
	if (s == 0) { hi = 0; lo = m; }
	else if (s == 32) { hi = m; lo = 0; }
	else { hi = C4_LSR (m, 32 - s); lo = m << s; }
	q = c4_udiv64 (hi, lo, d < 0 ? -d : d);
	*done = 1;
	return (a < 0) != (d < 0) ? (int32_t)(0u - q) : (int32_t)q;
}

/* (int32_t)((int64_t)a * b / c) */
int32_t c4_scale (int32_t a, int32_t b, int32_t c, int *done)
{
	uint32_t hi, lo, q;
	*done = 0;
	if (c == 0 || c >= 0x40000000 || c <= -0x40000000 || a == INT32_MIN || b == INT32_MIN)
		return 0;
	c4_umul64 (a, b, &hi, &lo);
	q = c4_udiv64 (hi, lo, c < 0 ? -c : c);
	*done = 1;
	return ((a < 0) != (b < 0)) != (c < 0) ? (int32_t)(0u - q) : (int32_t)q;
}
