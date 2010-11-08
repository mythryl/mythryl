/* acquire-proc.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-mp.h"
#include "cfun-proto-list.h"

/* _lib7_MP_acquire_proc:
 */
lib7_val_t _lib7_MP_acquire_proc (lib7_state_t *lib7_state, lib7_val_t arg)
{

#ifdef MP_SUPPORT
    return MP_AcquireProc (lib7_state, arg);
#else
    Die ("lib7_acquire_proc: no mp support\n");
#endif

} /* end of _lib7_MP_acquire_proc */



/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
