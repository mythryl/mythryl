/* runtime-state.c
 *
 */

#include "../config.h"

#include <stdarg.h>
#include "runtime-base.h"
#include "vproc-state.h"
#include "runtime-state.h"
#include "system-signals.h"
#include "tags.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "runtime-globals.h"
#include "gc.h"
#include "runtime-timer.h"
#include "runtime-limits.h"


vproc_state_t	*VProc[MAX_NUM_PROCS];
int		NumVProcs;


/* local routines */
static void InitVProcState (vproc_state_t *vsp);


/* AllocLib7state:
 */
lib7_state_t *AllocLib7state (bool_t is_boot, heap_params_t *heapParams)
{
    lib7_state_t	*lib7_state = NULL;
#ifdef MP_SUPPORT
    int		i;
#endif

#ifdef MP_SUPPORT

    for (i = 0; i < MAX_NUM_PROCS; i++) {
	if (((VProc[i] = NEW_CHUNK(vproc_state_t)) == NULL)
	||  ((lib7_state = NEW_CHUNK(lib7_state_t)) == NULL)) {
	    Die ("unable to allocate Lib7 state vectors");
	}
	VProc[i]->vp_state = lib7_state;
    }
    lib7_state = VProc[0]->vp_state;
#else
    if (((VProc[0] = NEW_CHUNK(vproc_state_t)) == NULL)
    ||  ((lib7_state = NEW_CHUNK(lib7_state_t)) == NULL)) {
	Die ("unable to allocate Lib7 state vector");
    }
    VProc[0]->vp_state = lib7_state;
#endif /* MP_SUPPORT */

  /* allocate and initialize the heap data structures */
    set_up_heap (lib7_state, is_boot, heapParams);

#ifdef MP_SUPPORT
  /* partition the allocation arena given by set_up_heap among the
   * MAX_NUM_PROCS processors.
   */
    NumVProcs = MAX_NUM_PROCS;
    PartitionAllocArena(VProc);
  /* initialize the per-processor Lib7 state */
    for (i = 0; i < MAX_NUM_PROCS; i++) {
	int	j;

	InitVProcState (VProc[i]);
      /* single timers are currently shared among multiple processors */
	if (i != 0) {
	    VProc[i]->vp_gcTime0 = VProc[0]->vp_gcTime0;
	    VProc[i]->vp_gcTime	= VProc[0]->vp_gcTime;
	}
    }
  /* initialize the first processor here */
    VProc[0]->vp_mpSelf = MP_ProcId ();
    VProc[0]->vp_mpState = MP_PROC_RUNNING;
#else
    InitVProcState (VProc[0]);
    NumVProcs = 1;
#endif /* MP_SUPPORT */

  /* initialize the timers */
  /** MP_SUPPORT note: for now, only proc 0 has timers **/
    reset_timers (VProc[0]);

    return lib7_state;

} /* end of AllocLib7state */

/* InitVProcState:
 */
static void InitVProcState (vproc_state_t *vsp)
{
    int		i;

    vsp->vp_heap			= vsp->vp_state->lib7_heap;
    vsp->vp_state->lib7_vproc		= vsp;
    vsp->vp_inLib7Flag			= FALSE;
    vsp->vp_handlerPending		= FALSE;
    vsp->vp_inSigHandler		= FALSE;
    vsp->vp_totalSigCount.nReceived	= 0;
    vsp->vp_totalSigCount.nHandled	= 0;
    vsp->vp_sigCode			= 0;
    vsp->vp_sigCount			= 0;
    vsp->vp_nextPendingSig		= MIN_SYSTEM_SIG;
    vsp->vp_gcSigState			= LIB7_SIG_IGNORE;
    vsp->vp_gcTime0			= NEW_CHUNK(Time_t);
    vsp->vp_gcTime			= NEW_CHUNK(Time_t);

    for (i = 0;  i < SIGMAP_SZ;  i++) {
	vsp->vp_sigCounts[i].nReceived = 0;
	vsp->vp_sigCounts[i].nHandled = 0;
    }

    /* initialize the Lib7 state, including the roots */
    InitLib7state (vsp->vp_state);
    vsp->vp_state->lib7_argument	= LIB7_void;
    vsp->vp_state->lib7_fate		= LIB7_void;
    vsp->vp_state->lib7_closure		= LIB7_void;
    vsp->vp_state->lib7_link_register	= LIB7_void;
    vsp->vp_state->lib7_program_counter	= LIB7_void;
    vsp->vp_state->lib7_exception_fate	= LIB7_void;
    vsp->vp_state->lib7_current_thread	= LIB7_void;
    vsp->vp_state->lib7_calleeSave[0]	= LIB7_void;
    vsp->vp_state->lib7_calleeSave[1]	= LIB7_void;
    vsp->vp_state->lib7_calleeSave[2]	= LIB7_void;

#ifdef MP_SUPPORT
    vsp->vp_mpSelf		= 0;
    vsp->vp_mpState		= MP_PROC_NO_PROC;
#endif

} /* end of InitVProcState */

/* InitLib7state:
 *
 * Initialize the Lib7 State vector.  Note that we do not initialize the root
 * registers here, since this is sometimes called when the roots are live (from
 * LIB7_ApplyFn).
 */
void InitLib7state (lib7_state_t *lib7_state)
{
    lib7_state->lib7_store_log		= LIB7_void;
#ifdef SOFT_POLL
    lib7_state->lib7_poll_event_is_pending		= FALSE;
    lib7_state->lib7_in_poll_handler	= FALSE;
#endif

} /* end of InitLib7state. */

/* SaveCState:
 *
 *    Build a return closure that will save a collection of Lib7 values
 * being used by C.  The Lib7 values are passed by reference, with NULL
 * as termination.
 */
void SaveCState (lib7_state_t *lib7_state, ...)
{
    va_list	    ap;
    int		    n, i;
    lib7_val_t	    *vp;

    va_start (ap, lib7_state);
    for (n = 0; (vp = va_arg(ap, lib7_val_t *)) != NULL;  n++)
	continue;
    va_end (ap);

    va_start (ap, lib7_state);
    LIB7_AllocWrite (lib7_state, 0, MAKE_DESC(n, DTAG_record));
    for (i = 1;  i <= n;  i++) {
	vp = va_arg (ap, lib7_val_t *);
        LIB7_AllocWrite (lib7_state, i, *vp);
    }
    lib7_state->lib7_calleeSave[0]   = LIB7_Alloc(lib7_state, n);
    lib7_state->lib7_fate    = PTR_CtoLib7(return_c);
    va_end (ap);

} /* end of SaveCState */

/* RestoreCState:
 *
 *    Restore a collection of Lib7 values from the return closure.
 */
void RestoreCState (lib7_state_t *lib7_state, ...)
{
    va_list	ap;
    int		n, i;
    lib7_val_t	*vp;
    lib7_val_t	savedState;

    va_start (ap, lib7_state);
    savedState = lib7_state->lib7_calleeSave[0];
    n = CHUNK_LEN(savedState);
    for (i = 0;  i < n;  i++) {
	vp = va_arg (ap, lib7_val_t *);
	*vp = REC_SEL(savedState, i);
    }
    va_end (ap);

} /* end of RestoreCState */



/* COPYRIGHT (c) 1993 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

