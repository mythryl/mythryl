/* unix-fault.c
 *
 * Common code for handling arithmetic traps.
 */

#if defined(__CYGWIN32__)

#include "../config.h"

#include "cygwin-fault.c"

#else

#include "../config.h"

#include "runtime-unixdep.h"
#include "signal-sysdep.h"
#include "runtime-base.h"
#include "vproc-state.h"
#include "runtime-state.h"
#include "runtime-globals.h"

/* this is temporary */
#define SELF_VPROC	(VProc[0])


/* local routines */
static SigReturn_t FaultHandler (/* int sig, SigInfo_t code, SigContext_t *scp */);


/* set_up_fault_handlers:
 */
void set_up_fault_handlers (lib7_state_t *lib7_state)
{

  /** Set up the DIVIDE_BY_ZERO and OVERFLOW faults **/
#ifdef SIG_FAULT1
    SIG_SetHandler (SIG_FAULT1, FaultHandler);
#endif
#ifdef SIG_FAULT2
    SIG_SetHandler (SIG_FAULT2, FaultHandler);
#endif

  /** Initialize the floating-point unit **/
    SIG_InitFPE ();

} /* end of set_up_fault_handlers */


/* FaultHandler:
 *
 * Handle arithmetic faults (e.g., divide by zero, integer overflow).
 */
#if defined(HAS_POSIX_SIGS) && defined(HAS_UCONTEXT)

static SigReturn_t FaultHandler (int signal, siginfo_t *si, void *c)
{
    ucontext_t	    *scp = (ucontext_t *)c;
    lib7_state_t	    *lib7_state = SELF_VPROC->vp_state;
    extern Word_t   request_fault[]; 
    int		    code = SIG_GetCode(si, scp);

#ifdef SIGNAL_DEBUG
    SayDebug ("Fault handler: sig = %d, inLib7 = %d\n",
	signal, SELF_VPROC->vp_inLib7Flag);
#endif

    if (! SELF_VPROC->vp_inLib7Flag) 
	Die ("bogus fault not in Lib7: sig = %d, code = %#x, pc = %#x)\n",
	    signal, SIG_GetCode(si, scp), SIG_GetPC(scp));

    /* Map the signal to the appropriate Lib7 exception. */
    if (INT_OVFLW(signal, code)) {
	lib7_state->lib7_fault_exception = OverflowId;
	lib7_state->lib7_faulting_program_counter = (Word_t)SIG_GetPC(scp);
    }
    else if (INT_DIVZERO(signal, code)) {
	lib7_state->lib7_fault_exception = DivId;
	lib7_state->lib7_faulting_program_counter = (Word_t)SIG_GetPC(scp);
    }
    else
	Die ("unexpected fault, signal = %d, code = %#x", signal, code);

    SIG_SetPC (scp, request_fault);

    SIG_ResetFPE (scp);

} /* end of FaultHandler */

#else

static SigReturn_t FaultHandler (
    int		    signal,
#if (defined(TARGET_PPC) && defined(OPSYS_LINUX))
    SigContext_t    *scp)
#else
    SigInfo_t	    info,
    SigContext_t    *scp)
#endif
{
    lib7_state_t	    *lib7_state = SELF_VPROC->vp_state;
    extern Word_t   request_fault[]; 
    int		    code = SIG_GetCode(info, scp);

#ifdef SIGNAL_DEBUG
    SayDebug ("Fault handler: sig = %d, inLib7 = %d\n",
	signal, SELF_VPROC->vp_inLib7Flag);
#endif

    if (! SELF_VPROC->vp_inLib7Flag) 
	Die ("bogus fault not in Lib7: sig = %d, code = %#x, pc = %#x)\n",
	    signal, SIG_GetCode(info, scp), SIG_GetPC(scp));

   /* Map the signal to the appropriate Lib7 exception. */
    if (INT_OVFLW(signal, code)) {
	lib7_state->lib7_fault_exception = OverflowId;
	lib7_state->lib7_faulting_program_counter = (Word_t)SIG_GetPC(scp);
    }
    else if (INT_DIVZERO(signal, code)) {
	lib7_state->lib7_fault_exception = DivId;
	lib7_state->lib7_faulting_program_counter = (Word_t)SIG_GetPC(scp);
    }
    else
	Die ("unexpected fault, signal = %d, code = %#x", signal, code);

    SIG_SetPC (scp, request_fault);

    SIG_ResetFPE (scp);

} /* end of FaultHandler */

#endif

#if ((defined(TARGET_RS6000) || defined(TARGET_PPC)) && defined(OPSYS_AIX))

/* SIG_GetCode:
 *
 * For  AIX, the overflow and divide by zero information is obtained
 * from information contained in the sigcontext package.
 */
static int SIG_GetCode (SigInfo_t code, SigContext_t *scp)
{
    struct fp_sh_info	FPInfo;

    fp_sh_info (scp, &FPInfo, sizeof(struct fp_sh_info));

    return FPInfo.trap;

} /* end of SIG_GetCode */

#endif

#endif /* !defined(__CYGWIN32__) */


/* COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

