/* release-proc.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-mp.h"
#include "cfun-proto-list.h"


/* _lib7_MP_max_procs:
 */
lib7_val_t _lib7_MP_max_procs (lib7_state_t *lib7_state, lib7_val_t arg)
{
#ifdef MP_SUPPORT
    return INT_CtoLib7(MP_MaxProcs ());
#else
    Die ("_lib7_MP_max_procs: no mp support\n");
#endif

} /* end of _lib7_MP_max_procs */



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
