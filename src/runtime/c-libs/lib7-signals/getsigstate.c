/* getsigstate.c
 *
 * This gets bound in:
 *
 *     src/lib/std/src/nj/internal-signals.pkg
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-state.h"
#include "runtime-signals.h"
#include "cfun-proto-list.h"

/* _lib7_Sig_getsigstate : Sysconst -> Int
 *
 */
lib7_val_t _lib7_Sig_getsigstate (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		state = GetSignalState (lib7_state->lib7_vproc, REC_SELINT(arg, 0));

    return INT_CtoLib7(state);

} /* end of _lib7_Sig_getsigstate */



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
