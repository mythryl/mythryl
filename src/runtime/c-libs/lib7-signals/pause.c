/* pause.c
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

/*
###   "More than iron, more than lead, more than gold, I need electricity.
###    I need it more than I need lamb or pork or lettuce or cucumber.
###    I need it for my dreams."
###
###            -- Racter (a program that sometimes writes poetry)
 */

/* _lib7_Sig_pause : Void -> Void
 *
 * Pause until the next signal.
 */
lib7_val_t _lib7_Sig_pause (lib7_state_t *lib7_state, lib7_val_t arg)
{

    PauseUntilSignal (lib7_state->lib7_vproc);			/*  PauseUntilSignal	def in   src/runtime/machine-dependent/unix-signal.c
								 */

    return LIB7_void;

} /* end of _lib7_Sig_pause */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
