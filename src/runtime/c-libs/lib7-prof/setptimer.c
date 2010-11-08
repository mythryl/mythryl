/* setptimer.c
 *
 * NOTE: this implementation is UNIX specific right now; I would like to
 * define an OS abstraction layer for interval timers, which would cover
 * both alarm timers and profiling, but I need to look at what other systems
 * do first.
 */

#include "../../config.h"

#ifdef OPSYS_UNIX
#  include "runtime-unixdep.h"
#if HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#endif
#include "runtime-base.h"
#include "lib7-c.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"
#include "profile.h"

/* _lib7_Prof_setptimer : Bool -> Void
 *
 * Turn the profile timer on/off.
 */
lib7_val_t _lib7_Prof_setptimer (lib7_state_t *lib7_state, lib7_val_t arg)
{
#ifdef HAS_SETITIMER
    struct itimerval	new_itv;
    int			status;


    if (arg == LIB7_false) {
	new_itv.it_interval.tv_sec	=
	new_itv.it_value.tv_sec		=
	new_itv.it_interval.tv_usec	=
	new_itv.it_value.tv_usec	= 0;
    }
    else if (ProfCntArray == LIB7_void) {
        return RAISE_ERROR(lib7_state, "no count array set");
    }
    else {
	new_itv.it_interval.tv_sec	=
	new_itv.it_value.tv_sec		= 0;
	new_itv.it_interval.tv_usec	=
	new_itv.it_value.tv_usec	= PROFILE_QUANTUM_US;
    }

    status = setitimer (ITIMER_VIRTUAL, &new_itv, NULL);

    CHECK_RETURN_UNIT(lib7_state, status);

#else
    return RAISE_ERROR(lib7_state, "time profiling not supported");
#endif

} /* end of _lib7_Prof_setptimer */



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

