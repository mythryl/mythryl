/* signal-util.c
 *
 * System independent support fns for
 * signals and software polling.
 */

#include "../config.h"

#include <stdio.h>
#include "runtime-base.h"
#include "runtime-limits.h"
#include "runtime-state.h"
#include "vproc-state.h"
#include "runtime-heap.h"
#include "runtime-signals.h"
#include "system-signals.h"


/* ChooseSignal:
 *
 * Caller guarantees that at least one Unix signal has been
 * seen at the C level but not yet handled at the Mythryl
 * level.  Our job is to find and return the number of
 * that signal plus the number of times it has fired at
 * the C level since last being handled at the Mythry level.
 *
 * Choose which signal to pass to the Mythryl-level handler
 * and set up the Mythryl state vector accordingly.
 *
 * This function gets called (only) from
 *
 *     src/runtime/main/run-runtime.c
 *
 * WARNING: This should be called with signals masked
 * to avoid race conditions.
 */
void ChooseSignal (vproc_state_t *vsp)
{
    int		i, j, delta;

    /* Scan the signal counts looking for
     * a signal that needs to be handled.
     *
     * The 'nReceived' field for a signal gets
     * incremented once for each incoming signal
     * in   CSigHandler()   in
     *
     *     src/runtime/machine-dependent/unix-signal.c
     *
     * Here we increment the matching 'nHandled' field
     * each time we invoke appropriate handling for that
     * signal;  thus, the difference between the two
     * gives the number of pending instances of that signal
     * currently needing to be handled.
     *
     * For fairness we scan for signals round-robin style, using
     *
     *     vsp->vp_nextPendingSig
     *
     * to remember where we left off scanning, so we can pick
     * up from there next time:	
     *
     */
    i = vsp->vp_nextPendingSig;
    j = 0;
    do {
	ASSERT (j++ < NUM_SIGS);

	i++;

	/* Wrap circularly around the signal vector:
	 */
	if (i == SIGMAP_SZ)
            i = MIN_SYSTEM_SIG;

	/* Does this signal have pending work? (Nonzero == "yes"):
	 */
	delta = vsp->vp_sigCounts[i].nReceived - vsp->vp_sigCounts[i].nHandled;

    } while (delta == 0);

    vsp->vp_nextPendingSig = i;		/* Next signal to scan on next call to this fn. */

    /* Record the signal to process
     * and how many times it has fired
     * since last being handled at the
     * Mythryl level:
     */
    vsp->vp_sigCode  = i;
    vsp->vp_sigCount = delta;

    /* Mark this signal as 'done':
     */
    vsp->vp_sigCounts[i].nHandled  += delta;
    vsp->vp_totalSigCount.nHandled += delta;

    #ifdef SIGNAL_DEBUG
    SayDebug ("ChooseSignal: sig = %d, count = %d\n",
    vsp->vp_sigCode, vsp->vp_sigCount);
    #endif
}


/* MakeResumeCont:
 *
 * Build the resume fate for a signal or poll event handler.
 * This closure contains the address of the resume entry-point and
 * the registers from the Lib7 state.
 *
 * At least 4K avail. heap assumed.
 *
 * This gets called from MakeHandlerArg() below,
 * and also from  src/runtime/main/run-runtime.c
 *
 */
lib7_val_t MakeResumeCont (lib7_state_t *lib7_state, lib7_val_t resume[])
{
    /* Allocate the resumption closure:
     */
    LIB7_AllocWrite(lib7_state,  0, MAKE_DESC(10, DTAG_record));
    LIB7_AllocWrite(lib7_state,  1, PTR_CtoLib7(resume));
    LIB7_AllocWrite(lib7_state,  2, lib7_state->lib7_argument);
    LIB7_AllocWrite(lib7_state,  3, lib7_state->lib7_fate);
    LIB7_AllocWrite(lib7_state,  4, lib7_state->lib7_closure);
    LIB7_AllocWrite(lib7_state,  5, lib7_state->lib7_link_register);
    LIB7_AllocWrite(lib7_state,  6, lib7_state->lib7_program_counter);
    LIB7_AllocWrite(lib7_state,  7, lib7_state->lib7_exception_fate);
    LIB7_AllocWrite(lib7_state,  8, lib7_state->lib7_calleeSave[0]); /* John Reppy says not to do: LIB7_AllocWrite(lib7_state,  8, lib7_state->lib7_current_thread); */
    LIB7_AllocWrite(lib7_state,  9, lib7_state->lib7_calleeSave[1]);
    LIB7_AllocWrite(lib7_state, 10, lib7_state->lib7_calleeSave[2]);
    /**/
    return LIB7_Alloc(lib7_state, 10);
}


/* MakeHandlerArg:
 *
 * Build the argument record for the Lib7 signal handler.
 * It has the type
 *
 *   sigHandler : (Int, Int, Fate(Void)) -> X
 *
 * where
 *     The first  argument is  the signal code,
 *     the second argument is  the signal count and
 *     the third  argument is  the resumption fate.
 *
 * The Lib7 signal handler should never return.
 * NOTE: maybe this should be combined with ChooseSignal???	XXX BUGGO FIXME
 */
lib7_val_t MakeHandlerArg (lib7_state_t *lib7_state, lib7_val_t resume[])
{
    lib7_val_t	resumeCont, arg;
    vproc_state_t *vsp = lib7_state->lib7_vproc;

    resumeCont = MakeResumeCont(lib7_state, resume);

    /* Allocate the Lib7 signal handler's argument record:
    */
    REC_ALLOC3(lib7_state, arg,
	INT_CtoLib7(vsp->vp_sigCode), INT_CtoLib7(vsp->vp_sigCount),
	resumeCont);

    #ifdef SIGNAL_DEBUG
    SayDebug ("MakeHandlerArg: resumeC = %#x, arg = %#x\n", resumeCont, arg);
    #endif

    return arg;
}


/* LoadResumeState:
 *
 * Load the Lib7 state with the state preserved
 * in resumption fate made by MakeResumeCont.
 */
void LoadResumeState (lib7_state_t *lib7_state)
{
    lib7_val_t	    *contClosure;

    #ifdef SIGNAL_DEBUG
    SayDebug ("LoadResumeState:\n");
    #endif

    contClosure = PTR_LIB7toC(lib7_val_t, lib7_state->lib7_closure);

    lib7_state->lib7_argument		= contClosure[1];
    lib7_state->lib7_fate		= contClosure[2];
    lib7_state->lib7_closure		= contClosure[3];
    lib7_state->lib7_link_register	= contClosure[4];
    lib7_state->lib7_program_counter	= contClosure[5];
    lib7_state->lib7_exception_fate	= contClosure[6];

    /* John (Reppy) says current_thread
    should not be included here...
    lib7_state->lib7_current_thread	= contClosure[7];
    */

    lib7_state->lib7_calleeSave[0]	= contClosure[7];
    lib7_state->lib7_calleeSave[1]	= contClosure[8];
    lib7_state->lib7_calleeSave[2]	= contClosure[9];
}



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

