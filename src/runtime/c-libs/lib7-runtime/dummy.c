/* dummy.c
 *
 * This is a dummy run-time routine for when we would like to call
 * a null C function.
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "cfun-proto-list.h"

lib7_val_t   _lib7_runtime_dummy   (   lib7_state_t*   lib7_state,
                                         lib7_val_t      arg
                                     )
{
    /* _lib7_runtime_dummy : String -> Void
     *
     * The string argument can be used as a unique marker.
     */

    /*
      char	*s = STR_LIB7toC(arg);
    */

    return LIB7_void;
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

