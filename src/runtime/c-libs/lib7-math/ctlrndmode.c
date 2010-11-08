/* ctlrndmode.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "fp-dep.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"
#include "lib7-c.h"

#ifndef NO_ROUNDING_MODE_CTL
/* Mapping between the Lib7 and C representations of rounding modes. */
#if defined(RMODE_C_EQ_LIB7)
#  define RMODE_CtoLib7(m)	INT_CtoLib7(m)
#  define RMODE_LIB7toC(m)	INT_LIB7toC(m)
#else
#  define RMODE_CtoLib7(m)						\
      (RMODE_EQ(m, FE_TONEAREST) ? INT_CtoLib7(0)				\
	: (RMODE_EQ(m, FE_TOWARDZERO) ? INT_CtoLib7(1)			\
	  : (RMODE_EQ(m, FE_UPWARD) ? INT_CtoLib7(2) : INT_CtoLib7(3))))
static fe_rnd_mode_t ModeMap[4] = {
	FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD
    };
#  define RMODE_LIB7toC(m)	ModeMap[INT_LIB7toC(m)]
#endif
#endif /* !NO_ROUNDING_MODE_CTL */

/* _lib7_Math_ctlrndmode : int option -> int
 *
 * Get/set the rounding mode; the values are interpreted as follows:
 *
 *	0	To nearest
 *	1	To zero
 *	2	To +Inf
 *	3	To -Inf
 */
lib7_val_t _lib7_Math_ctlrndmode (lib7_state_t *lib7_state, lib7_val_t arg)
{
#ifdef NO_ROUNDING_MODE_CTL
  return RAISE_ERROR(lib7_state, "Rounding mode control not supported");

#else
    if (arg == OPTION_NONE) {
	fe_rnd_mode_t	res = fegetround();
	return RMODE_CtoLib7(res);
    }
    else {
	fe_rnd_mode_t	m = RMODE_LIB7toC(OPTION_get(arg));
	fe_rnd_mode_t	res = fesetround(m);
	return RMODE_CtoLib7(res);
    }
#endif

} /* end of _lib7_Math_ctlrndmode */



/* COPYRIGHT (c) 1996 AT&T Research.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
