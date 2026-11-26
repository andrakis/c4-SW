// sw-math.h -- c4's port: JFBuild's 64-bit helpers from 32-bit operations
// (sw-math.c), for pragmas.h under c4lc
#ifndef C4SW_MATH_H
#define C4SW_MATH_H 1

#include <stdint.h>

int32_t c4_mulscale (int32_t a, int32_t b, int32_t s);		/* 0 <= s <= 32 */
int32_t c4_dmulscale (int32_t a, int32_t b, int32_t c, int32_t d, int32_t s);
int32_t c4_tmulscale (int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f, int32_t s);
int32_t c4_divscale (int32_t a, int32_t d, int32_t s, int *done);
int32_t c4_scale (int32_t a, int32_t b, int32_t c, int *done);

#endif
