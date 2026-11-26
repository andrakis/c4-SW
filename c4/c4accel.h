/* c4accel.h -- c4's port: Shadow Warrior's accelerators, c4/accel.mjs,
   from the game's side (c4's libjs/guest/accel.h, docs/accel.md). On a
   machine that has them the host runs these in the game's own JavaScript,
   one instruction each; elsewhere, and in gcc's build, the C runs as
   before. accel_sw is the first of the module's numbers (0 for none, -1
   until asked); SW_ACCEL() asks once. The numbers are the module's
   operations in its order. As Duke Nukem 3D's c4/c4accel.h. */
#ifndef C4SW_ACCEL_H
#define C4SW_ACCEL_H

#include <stdint.h>
#include "accel.h"

enum {
	SA_VLINE, SA_MVLINE, SA_TVLINE, SA_HLINE, SA_MHLINE, SA_THLINE,
	SA_SPRV, SA_MSPRV, SA_TSPRV, SA_SLOPE,
	SA_MULSCALE, SA_DMULSCALE, SA_TMULSCALE, SA_SCALE, SA_DIVSCALE
};

extern int32_t accel_sw;
int32_t accel_sw_base (void);
#define SW_ACCEL() (accel_sw >= 0 ? accel_sw : accel_sw_base ())

#ifdef __c4_inttypes__
#define SA_OP(op) (accel_sw + (op))
/* a-c.c's loops, with the globals they read as arguments */
#define sw_vline(vinc, pal, cnt, vplc, buf, p, logy, bpl) \
	__c4_opcode ((vinc), (pal), (cnt), (vplc), (buf), (p), (logy), (bpl), SA_OP (SA_VLINE))
#define sw_mvline(vinc, pal, cnt, vplc, buf, p, logy, bpl) \
	__c4_opcode ((vinc), (pal), (cnt), (vplc), (buf), (p), (logy), (bpl), SA_OP (SA_MVLINE))
#define sw_tvline(vinc, pal, cnt, vplc, buf, p, logy, bpl, trans, mode) \
	__c4_opcode ((vinc), (pal), (cnt), (vplc), (buf), (p), (logy), (bpl), (trans), (mode), SA_OP (SA_TVLINE))
#define sw_hline(cnt, pal, by, bx, buf, p, logx, logy, bxinc, byinc) \
	__c4_opcode ((cnt), (pal), (by), (bx), (buf), (p), (logx), (logy), (bxinc), (byinc), SA_OP (SA_HLINE))
#define sw_mhline(buf, bx, cnt, by, p, logx, logy, pal, a1, a2) \
	__c4_opcode ((buf), (bx), (cnt), (by), (p), (logx), (logy), (pal), (a1), (a2), 0, 0, SA_OP (SA_MHLINE))
#define sw_thline(buf, bx, cnt, by, p, logx, logy, pal, a1, a2, trans, mode) \
	__c4_opcode ((buf), (bx), (cnt), (by), (p), (logx), (logy), (pal), (a1), (a2), (trans), (mode), SA_OP (SA_THLINE))
#define sw_sprv(op, bx, by, cnt, buf, p, pal, bxinc, byinc, ysiz, bpl, trans, mode) \
	__c4_opcode ((bx), (by), (cnt), (buf), (p), (pal), (bxinc), (byinc), (ysiz), (bpl), (trans), (mode), SA_OP (op))
#define sw_slope(p, slopal, cnt, bx, by, bz, bzinc, gx3, gy3, buf, logx, logy, pinc, recip) \
	__c4_opcode ((p), (slopal), (cnt), (bx), (by), (bz), (bzinc), (gx3), (gy3), (buf), (logx), (logy), \
		(pinc), (recip), SA_OP (SA_SLOPE))
/* pragmas.h's, exactly as the int64_t code */
#define sw_mulscale(a, b, s) __c4_opcode ((a), (b), (s), SA_OP (SA_MULSCALE))
#define sw_dmulscale(a, b, c, d, s) __c4_opcode ((a), (b), (c), (d), (s), SA_OP (SA_DMULSCALE))
#define sw_tmulscale(a, b, c, d, e, f, s) __c4_opcode ((a), (b), (c), (d), (e), (f), (s), SA_OP (SA_TMULSCALE))
#define sw_scale(a, b, c) __c4_opcode ((a), (b), (c), SA_OP (SA_SCALE))
#define sw_divscale(a, b, s) __c4_opcode ((a), (b), (s), SA_OP (SA_DIVSCALE))
#endif

#endif
