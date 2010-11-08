/* win32-signal.c
 *
 * when "signals" are supported in win32, they'll go here.
 */

#include "../config.h"

#include "signal-sysdep.h"
#include "runtime-base.h"
#include "runtime-limits.h"
#include "runtime-state.h"
#include "vproc-state.h"
#include "runtime-heap.h"
#include "runtime-signals.h"
#include "system-signals.h"
#include "runtime-globals.h"

#include "win32-sigtable.c"

#ifndef MP_SUPPORT
#define SELF_VPROC	(VProc[0])
#else
/** for MP_SUPPORT, we'll use SELF_VPROC for now **/
#define SELF_VPROC	(VProc[0])
#endif

/* ListSignals:
 */
lib7_val_t ListSignals (lib7_state_t *lib7_state)
{
#ifdef WIN32_DEBUG
    SayDebug("win32:ListSignals: returning dummy signal list\n");
#endif
    return LIB7_SysConstList (lib7_state, &SigTable);
} 

/* PauseUntilSignal:
 *
 * Suspend the given VProc until a signal is received.
 */
void PauseUntilSignal (vproc_state_t *vsp)
{
#ifdef WIN32_DEBUG
    SayDebug("win32:PauseUntilSignal: returning without pause\n");
#endif
} 

/* SetSignalState:
 */
void SetSignalState (vproc_state_t *vsp, int sigNum, int sigState)
{
#ifdef WIN32_DEBUG
    SayDebug("win32:SetSignalState: not setting state for signal %d\n",sigNum);
#endif
}


/* GetSignalState:
 */
int GetSignalState (vproc_state_t *vsp, int sigNum)
{
#ifdef WIN32_DEBUG
    SayDebug("win32:GetSignalState: returning state for signal %d as LIB7_SIG_DEFAULT\n",sigNum);
#endif
    return LIB7_SIG_DEFAULT;
}  


/* SetSignalMask:
 *
 * Set the signal mask to the given list of signals.  The sigList has the
 * type: "sysconst list option", with the following semantics (see
 * signals.pkg):
 *	NULL	-- the empty mask
 *	THE[]	-- mask all signals
 *	THE l	-- the signals in l are the mask
 */
void SetSignalMask (lib7_val_t sigList)
{
#ifdef WIN32_DEBUG
    SayDebug("win32:SetSigMask: not setting mask\n");
#endif
}


/* GetSignalMask:
 *
 * Return the current signal mask (only those signals supported by Lib7); like
 * SetSignalMask, the result has the following semantics:
 *	NULL	-- the empty mask
 *	THE[]	-- mask all signals
 *	THE l	-- the signals in l are the mask
 */
lib7_val_t GetSignalMask (lib7_state_t *lib7_state)
{
#ifdef WIN32_DEBUG
    SayDebug("win32:GetSignalMask: returning mask as NULL\n");
#endif
    return OPTION_NONE;
}

/* end of win32-signal.c */


/* COPYRIGHT (c) 1996 Bell Laboratories, Lucent Technologies
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

