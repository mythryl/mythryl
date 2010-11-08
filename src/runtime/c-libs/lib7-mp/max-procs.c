/* max-procs.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-mp.h"
#include "cfun-proto-list.h"


/* _lib7_MP_release_proc:
 */
lib7_val_t _lib7_MP_release_proc (lib7_state_t *lib7_state, lib7_val_t arg)
{

#ifdef MP_SUPPORT
    MP_ReleaseProc(lib7_state);  /* should not return */
    Die ("_lib7_MP_release_proc: call unexpectedly returned\n");
#else
    Die ("_lib7_MP_release_proc: no mp support\n");
#endif

} /* end of _lib7_MP_release_proc */


/* COPYRIGHT (c) 1994 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
