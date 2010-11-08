/* setsigstate.c
 *
 * This gets bound in:
 *
 *     src/lib/std/src/nj/internal-signals.pkg
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "runtime-signals.h"
#include "cfun-proto-list.h"

/* _lib7_Sig_setsigstate : (Sysconst, Int) -> Void
 *
 */
lib7_val_t _lib7_Sig_setsigstate (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	sig = REC_SEL(arg, 0);

    SetSignalState (lib7_state->lib7_vproc, REC_SELINT(sig, 0), REC_SELINT(arg, 1));	/* SetSignalState	def in    src/runtime/machine-dependent/unix-signal.c
											*/
    return LIB7_void;
}



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
