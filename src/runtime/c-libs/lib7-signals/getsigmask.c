/* getsigmask.c
 *
 * This gets bound in:
 *
 *     src/lib/std/src/nj/internal-signals.pkg
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-signals.h"
#include "cfun-proto-list.h"

/* _lib7_Sig_getsigmask : Void -> Null_Or( List( Sysconst ) )
 *
 */
lib7_val_t _lib7_Sig_getsigmask (lib7_state_t *lib7_state, lib7_val_t arg)
{
  return GetSignalMask (lib7_state);		/* See, e.g., src/runtime/machine-dependent/unix-signal.c */
}



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
