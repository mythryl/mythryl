/* itick.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

lib7_val_t   _lib7_runtime_itick   (   lib7_state_t*   lib7_state,
                                         lib7_val_t      arg
                                     )
{
    /* _lib7_runtime_itick : Void -> (int * int) */

  return RAISE_ERROR( lib7_state, "itick unimplemented");
}



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
