// setitimer.c
//
// "set interval timer" -- have Linux kernel generate periodic
// SIGALRM signals to us, typically at 50Hz.  We use this to
// drive preemptive thread scheduling in support of concurrent
// threads -- see  set_sigalrm_frequency()  call in
//
//      src/lib/src/lib/thread-kit/src/core-thread-kit/thread-scheduler.pkg
//
// which invokes the interface logic in
//
//      src/lib/std/src/nj/set-sigalrm-frequency.pkg 
//
// NOTE: This implementation is UNIX specific right now.
//
// I would like to define an OS abstraction layer
// for interval timers, which would cover both
// alarm timers and profiling, but I need to look
// at what other systems do first.


#include "../../config.h"

#include "runtime-base.h"
#ifdef OPSYS_UNIX
#  include "system-dependent-unix-stuff.h"
#if HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#elif defined(OPSYS_WIN32)
#  include "win32-timers.h"
#endif
#include "lib7-c.h"
#include "runtime-values.h"
#include "make-strings-and-vectors-etc.h"
#include "cfun-proto-list.h"

#include "../socket/log-if.h"


// One of the library bindings exported via
//     src/c/lib/heap/cfun-list.h
// and thence
//     src/c/lib/heap/libmythryl-heap.c



Val   _lib7_runtime_set_sigalrm_frequency   (Task* task,  Val arg)   {
    //=======================
    //
    // Mythryl type:   Null_Or( (Int, Int) ) -> Void
    //
    // Set the interval timer; NULL means disable the timer
    //
    // This fn gets bound as   set_sigalrm_frequency'   in:
    //
    //     src/lib/std/src/nj/set-sigalrm-frequency.pkg

#ifdef HAS_SETITIMER
    struct itimerval	new_itv;
    int			status;
    Val		tmp;

    if (arg == OPTION_NULL) {

log_if("setitimer.c: Turning OFF SIGALRM interval timer\n");
        // Turn off the timer:
        //
	new_itv.it_interval.tv_sec	=
	new_itv.it_value.tv_sec		=
	new_itv.it_interval.tv_usec	=
	new_itv.it_value.tv_usec	= 0;

    } else {

        // Turn on the timer:
        //
	tmp = OPTION_GET(arg);

	new_itv.it_interval.tv_sec	=
	new_itv.it_value.tv_sec		= TUPLE_GET_INT1(tmp, 0);

	new_itv.it_interval.tv_usec	=
	new_itv.it_value.tv_usec	= GET_TUPLE_SLOT_AS_INT(tmp, 1);

log_if("setitimer.c: Turning ON SIGALRM interval itimer, sec,usec = (%d,%d)\n",new_itv.it_value.tv_sec, new_itv.it_value.tv_usec);
    }

    status = setitimer (ITIMER_REAL, &new_itv, NULL);			// See setitimer(2), Linux Reference Manual.

    CHECK_RETURN_UNIT(task, status);

#elif defined(OPSYS_WIN32)

    if (arg == OPTION_NULL) {

	if (win32StopTimer())	  return HEAP_VOID;
	else                      return RAISE_ERROR( task, "win32 setitimer: couldn't stop timer");

    } else {

	Val	tmp = OPTION_GET(arg);
	int		mSecs = TUPLE_GET_INT1(tmp,0) * 1000 + GET_TUPLE_SLOT_AS_INT(tmp,1) / 1000;

	if (mSecs <= 0)   return RAISE_ERROR( task, "win32 setitimer: invalid resolution");
	else {
	    if (win32StartTimer(mSecs))	   return HEAP_VOID;
	    else                           return RAISE_ERROR( task, "win32 setitimer: couldn't start timer");
	}
    }
#else
    return RAISE_ERROR( task, "setitimer not supported");
#endif

}


// COPYRIGHT (c) 1995 AT&T Bell Laboratories.
// Subsequent changes by Jeff Prothero Copyright (c) 2010-2011,
// released under Gnu Public Licence version 3.


