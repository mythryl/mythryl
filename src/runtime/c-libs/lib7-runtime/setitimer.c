/* setitimer.c
 *
 * "set interval timer" -- have Linux kernel generate periodic
 * SIGALRM signals to us, typically at 50Hz.  We use this to
 * drive preemptive thread scheduling in support of concurrent
 * threads -- see  set_interval_timer()  call in
 *
 *      src/lib/src/lib/thread-kit/src/core-thread-kit/thread-scheduler.pkg
 *
 * which invokes the interface logic in
 *
 *      src/lib/std/src/nj/interval-timer.pkg 
 *
 * NOTE: This implementation is UNIX specific right now.
 *
 * I would like to define an OS abstraction layer
 * for interval timers, which would cover both
 * alarm timers and profiling, but I need to look
 * at what other systems do first.
 */

#include "../../config.h"

#include "runtime-base.h"
#ifdef OPSYS_UNIX
#  include "runtime-unixdep.h"
#if HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#elif defined(OPSYS_WIN32)
#  include "win32-timers.h"
#endif
#include "lib7-c.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"

#include "../lib7-socket/print-if.h"

lib7_val_t   _lib7_runtime_setitimer   (   lib7_state_t*   lib7_state,
                                           lib7_val_t      arg
                                       )
{
    /* _lib7_runtime_setitimer: Null_Or( (Int, Int) -> Void
     *
     * Set the interval timer; NULL means disable the timer
     */


#ifdef HAS_SETITIMER
    struct itimerval	new_itv;
    int			status;
    lib7_val_t		tmp;

    if (arg == OPTION_NONE) {

print_if("setitimer.c: Turning OFF SIGALRM interval timer\n");
        /* Turn off the timer:
        */
	new_itv.it_interval.tv_sec	=
	new_itv.it_value.tv_sec		=
	new_itv.it_interval.tv_usec	=
	new_itv.it_value.tv_usec	= 0;

    } else {

        /* Turn on the timer:
        */
	tmp = OPTION_get(arg);

	new_itv.it_interval.tv_sec	=
	new_itv.it_value.tv_sec		= REC_SELINT32(tmp, 0);

	new_itv.it_interval.tv_usec	=
	new_itv.it_value.tv_usec	= REC_SELINT(tmp, 1);

print_if("setitimer.c: Turning ON SIGALRM interval itimer, sec,usec = (%d,%d)\n",new_itv.it_value.tv_sec, new_itv.it_value.tv_usec);
    }

    status = setitimer (ITIMER_REAL, &new_itv, NULL);			/* See setitimer(2), Linux Reference Manual. */

    CHECK_RETURN_UNIT(lib7_state, status);

#elif defined(OPSYS_WIN32)

    if (arg == OPTION_NONE) {

	if (win32StopTimer())	  return LIB7_void;
	else                      return RAISE_ERROR( lib7_state, "win32 setitimer: couldn't stop timer");

    } else {

	lib7_val_t	tmp = OPTION_get(arg);
	int		mSecs = REC_SELINT32(tmp,0) * 1000 + REC_SELINT(tmp,1) / 1000;

	if (mSecs <= 0)   return RAISE_ERROR( lib7_state, "win32 setitimer: invalid resolution");
	else {
	    if (win32StartTimer(mSecs))	   return LIB7_void;
	    else                           return RAISE_ERROR( lib7_state, "win32 setitimer: couldn't start timer");
	}
    }
#else
    return RAISE_ERROR( lib7_state, "setitimer not supported");
#endif

}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

