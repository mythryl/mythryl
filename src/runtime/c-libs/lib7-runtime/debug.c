/* debug.c
 *
 * Print a string out to the debug stream.
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "cfun-proto-list.h"


lib7_val_t   _lib7_runtime_debug   (   lib7_state_t*   lib7_state,
                                         lib7_val_t      arg
                                     )
{
    /* _lib7_runtime_debug : String -> Void */

    SayDebug (STR_LIB7toC(arg));

    return LIB7_void;
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

