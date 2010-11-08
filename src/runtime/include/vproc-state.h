/* vproc-state.h
 *
 * This is the state of a virtual processor.
 */



/*
###                "Light is the task when many share the toil."
###
###                                 -- Homer, circa 750BC
 */



#ifndef _VPROC_STATE_
#define _VPROC_STATE_

#ifndef _LIB7_BASE_
#include "runtime-base.h"
#endif

#ifndef _LIB7_SIGNALS_
#include "runtime-signals.h"
#endif

#ifndef _SYSTEM_SIGNALS_
#include "system-signals.h"
#endif

#ifndef _LIB7_TIMER_
#include "runtime-timer.h"
#endif

#if defined(MP_SUPPORT) && (! defined(_LIB7_MP_))
#include "runtime-mp.h"
#endif


/** The Virtual processor state vector **/
struct vproc_state {
    heap_t	*vp_heap;	        /* The heap for this Lib7 task                  */
    lib7_state_t*vp_state;	        /* The state of the Lib7 task that is           */
				        /* running on this VProc.  Eventually           */
				        /* we will support multiple Lib7 tasks          */
				        /* per VProc.                                   */
				        /* Signal related fields:                       */
    bool_t	vp_inLib7Flag;		/* True while executing Lib7 code               */
    bool_t	vp_handlerPending;	/* Is there a signal handler pending?           */
    bool_t	vp_inSigHandler;	/* Is an Lib7 signal handler active?            */
    sig_count_t	vp_totalSigCount;	/* summary count for all system signals         */
    int		vp_sigCode;		/* the code and count of the next               */
    int		vp_sigCount;		/* signal to handle.                            */
    sig_count_t	vp_sigCounts[SIGMAP_SZ];/* counts of signals.                           */
    int		vp_nextPendingSig;	/* the index in sigCounts of the next           */
					/* signal to handle.                            */
    int		vp_gcSigState;		/* The state of the GC signal handler           */
    Time_t	*vp_gcTime0;	        /* The cumulative CPU time at the start of      */
				        /* the last GC (see src/runtime/main/timers.c). */
    Time_t	*vp_gcTime;	        /* The cumulative GC time.                      */
    Unsigned32_t vp_limitPtrMask;       /* for raw-C-call interface                     */
#ifdef MP_SUPPORT
    mp_pid_t	vp_mpSelf;	        /* the owning process's ID                      */
    vproc_status_t vp_mpState;	        /* proc state (see runtime-mp.h)                */
#endif
};

#endif /* !_VPROC_STATE_ */



/* COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

