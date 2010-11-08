/* unix-signal.c
 *
 * Unix specific code to support Mythryl-level handling of unix signals.
 */

#include "../config.h"

#include "runtime-unixdep.h"
#include "signal-sysdep.h"
#include "runtime-base.h"
#include "runtime-limits.h"
#include "runtime-state.h"
#include "vproc-state.h"
#include "runtime-heap.h"
#include "runtime-signals.h"
#include "system-signals.h"
#include "runtime-globals.h"

#include "../c-libs/lib7-socket/print-if.h"


/* The generated sys_const_t table for UNIX signals */
#include "unix-sigtable.c"


#ifndef MP_SUPPORT
#define SELF_VPROC	(VProc[0])
#else
/** for MP_SUPPORT, we'll use SELF_VPROC for now **/
#define SELF_VPROC	(VProc[0])
#endif


#ifdef USE_ZERO_LIMIT_PTR_FN
Addr_t		SavedPC;
extern		ZeroLimitPtr[];
#endif

/* local routines
 */
static SigReturn_t   CSigHandler   (/* int sig, SigInfo_t info, SigContext_t *scp */);


lib7_val_t   ListSignals   (lib7_state_t *lib7_state)		/* Called from src/runtime/c-libs/lib7-signals/listsignals.c	*/
{
    return LIB7_SysConstList (lib7_state, &SigTable);		/* See src/runtime/gc/runtime-heap.c				*/
}

void   PauseUntilSignal   (vproc_state_t *vsp) {

    /* Suspend the given VProc
     * until a signal is received:
     */
    pause ();							/* pause() is a clib function, see pause(2).			*/
}

void   SetSignalState   (vproc_state_t *vsp, int sig_num, int sigState) {

    /* QUESTIONS:
     *
     * If we disable a signal that has pending signals,
     * should the pending signals be discarded?
     *
     * How do we keep track of the state
     * of non-UNIX signals (e.g., GC)?
     */

    switch (sig_num) {

    case RUNSIG_GC:
	vsp->vp_gcSigState = sigState;
	break;

    default:
	if (IS_SYSTEM_SIG(sig_num)) {

	    switch (sigState) {

	    case LIB7_SIG_IGNORE:
		SIG_SetIgnore (sig_num);
		break;

	    case LIB7_SIG_DEFAULT:
		SIG_SetDefault (sig_num);
		break;

	    case LIB7_SIG_ENABLED:
		SIG_SetHandler (sig_num, CSigHandler);			/* SIG_SetHandler 	#define in   src/runtime/machine-dependent/signal-sysdep.h			*/
		break;

	    default:
		Die ("bogus signal state: sig = %d, state = %d\n",
		    sig_num, sigState);
	    }

	} else {

            Die ("SetSignalState: unknown signal %d\n", sig_num);
	}
    }
}


int
GetSignalState (vproc_state_t *vsp, int sig_num)
{
    switch (sig_num) {

    case RUNSIG_GC:
	return vsp->vp_gcSigState;

    default:
	if (IS_SYSTEM_SIG(sig_num)) {

	    SigReturn_t		(*handler)();
	    SIG_GetHandler (sig_num, handler);

	    if (handler == SIG_IGN)
		return LIB7_SIG_IGNORE;
	    else if (handler == SIG_DFL)
		return LIB7_SIG_DEFAULT;
	    else
		return LIB7_SIG_ENABLED;
	}
	else Die ("GetSignalState: unknown signal %d\n", sig_num);
    }
}


#if defined(HAS_POSIX_SIGS) && defined(HAS_UCONTEXT)

static SigReturn_t   CSigHandler   (int sig, siginfo_t *si, void *c)
{
    /* This is the C signal handler for
     * signals that are to be passed to
     * the Mythryl level via signal_handler in
     *
     *     src/lib/std/src/nj/internal-signals.pkg
     */

    ucontext_t	    *scp = (ucontext_t *)c;
    vproc_state_t   *vsp = SELF_VPROC;

    /* Remember that we have seen signal number 'sig'.
     *
     * This will eventually get noticed by  ChooseSignal()  in
     *
     *     src/runtime/machine-dependent/signal-util.c
     */
    vsp->vp_sigCounts[sig].nReceived++;
    vsp->vp_totalSigCount.nReceived++;

print_if("unix-signal.c/CSigHandler: signal d=%d nRecieved %d nHandled d=%d diff d=%d\n", sig, vsp->vp_sigCounts[sig].nReceived, vsp->vp_sigCounts[sig].nHandled, vsp->vp_sigCounts[sig].nReceived - vsp->vp_sigCounts[sig].nHandled   );
#ifdef SIGNAL_DEBUG
SayDebug ("CSigHandler: sig = %d, pending = %d, inHandler = %d\n", sig, vsp->vp_handlerPending, vsp->vp_inSigHandler);
#endif

    /* The following line is needed only when
     * currently executing "pure" C code, but
     * doing it anyway in all other cases will
     * not hurt:
     */
    vsp->vp_limitPtrMask = 0;

    if (vsp->vp_inLib7Flag && (! vsp->vp_handlerPending) && (! vsp->vp_inSigHandler)) {
	vsp->vp_handlerPending = TRUE;
#ifdef USE_ZERO_LIMIT_PTR_FN
	SIG_SavePC(vsp->vp_state, scp);
	SIG_SetPC(scp, ZeroLimitPtr);
#else /* we can adjust the heap limit directly */
	SIG_ZeroLimitPtr(scp);
#endif
    }
}

#else

static SigReturn_t   CSigHandler
(
    int		    sig,
#if (defined(TARGET_PPC) && defined(OPSYS_LINUX))
    SigContext_t    *scp
#else
    SigInfo_t	    info,
    SigContext_t    *scp
#endif
) {
#if defined(OPSYS_LINUX) && defined(TARGET_X86) && defined(USE_ZERO_LIMIT_PTR_FN)
    SigContext_t    *scp = &sc;
#endif
    vproc_state_t   *vsp = SELF_VPROC;

    vsp->vp_sigCounts[sig].nReceived++;
    vsp->vp_totalSigCount.nReceived++;

#ifdef SIGNAL_DEBUG
SayDebug ("CSigHandler: sig = %d, pending = %d, inHandler = %d\n",
sig, vsp->vp_handlerPending, vsp->vp_inSigHandler);
#endif

    /* The following line is needed only when
     * currently executing "pure" C code, but
     * doing it anyway in all other cases will
     * not hurt:
     */
    vsp->vp_limitPtrMask = 0;

    if (vsp->vp_inLib7Flag && (! vsp->vp_handlerPending) && (! vsp->vp_inSigHandler)) {

	vsp->vp_handlerPending = TRUE;

#ifdef USE_ZERO_LIMIT_PTR_FN

	SIG_SavePC(vsp->vp_state, scp);
	SIG_SetPC(scp, ZeroLimitPtr);

#else
        /* We can adjust the heap limit directly:
        */
	SIG_ZeroLimitPtr(scp);
#endif
    }
}

#endif


/* SetSignalMask:
 *
 * Set the signal mask to the given list of signals.
 * The sigList has the type
 *
 *     "sysconst list option"
 *
 * with the following semantics -- see src/lib/std/src/nj/signals.pkg
 *
 *	NULL	-- the empty mask
 *	THE[]	-- mask all signals
 *	THE l	-- the signals in l are the mask
 */
void SetSignalMask (lib7_val_t sigList)
{
    SigMask_t	mask;
    int		i;

    SIG_ClearMask(mask);

    if (sigList != OPTION_NONE) {
	sigList = OPTION_get(sigList);

	if (LIST_isNull(sigList)) {

	    /* THE [] -- mask all signals
            */
	    for (i = 0;  i < NUM_SYSTEM_SIGS;  i++) {
		SIG_AddToMask(mask, SigInfo[i].id);
	    }

	} else {

	    while (sigList != LIST_nil) {
		lib7_val_t	car = LIST_hd(sigList);
		int		sig = REC_SELINT(car, 0);
		SIG_AddToMask(mask, sig);
		sigList = LIST_tl(sigList);
	    }
	}
    }

    /* Do the actual host OS syscall to change the signal mask.
     * This is our only invocation of this syscall:
     */
print_if("unix-signal.c/SetSignalMask: setting host signal mask for process to x=%x\n", mask );
    SIG_SetMask(mask);
}


/* GetSignalMask:
 *
 * Return the current signal mask (only those signals supported by Lib7); like
 * SetSignalMask, the result has the following semantics:
 *	NULL	-- the empty mask
 *	THE[]	-- mask all signals
 *	THE l	-- the signals in l are the mask
 */
lib7_val_t GetSignalMask (lib7_state_t *lib7_state)		/* Called from src/runtime/c-libs/lib7-signals/getsigmask.c */
{
    SigMask_t	mask;
    lib7_val_t	name, sig, sigList, result;
    int		i, n;

    SIG_GetMask(mask);

    /* Count the number of masked signals:
    */
    for (i = 0, n = 0;  i < NUM_SYSTEM_SIGS;  i++) {
	if (SIG_isSet(mask, SigInfo[i].id)) n++;
    }

    if (n == 0)
	return OPTION_NONE;
    else if (n == NUM_SYSTEM_SIGS)
	sigList = LIST_nil;
    else {
	for (i = 0, sigList = LIST_nil;  i < NUM_SYSTEM_SIGS;  i++) {
	    if (SIG_isSet(mask, SigInfo[i].id)) {
		name = LIB7_CString (lib7_state, SigInfo[i].name);
		REC_ALLOC2(lib7_state, sig, INT_CtoLib7(SigInfo[i].id), name);
		LIST_cons(lib7_state, sigList, sig, sigList);
	    }
	}
    }

    OPTION_SOME(lib7_state, result, sigList);
    return result;
}


/* COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

