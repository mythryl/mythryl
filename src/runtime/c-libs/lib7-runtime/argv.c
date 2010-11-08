/* argv.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"



lib7_val_t   _lib7_Proc_argv   (   lib7_state_t*   lib7_state,
                                     lib7_val_t      arg
                                 )
{
    return LIB7_CStringList (lib7_state, commandline_arguments);
}



/* COPYRIGHT (c) 1992 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
