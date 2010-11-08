/* cmd-name.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "cfun-proto-list.h"

lib7_val_t   _lib7_Proc_cmd_name   (   lib7_state_t*   lib7_state,
                                         lib7_val_t      arg
                                     )
{
    /* _lib7_Proc_cmd_name : Void -> String */

    return LIB7_CString( lib7_state, Lib7CommandName );
}



/* COPYRIGHT (c) 1997 Bell Labs, Lucent Technologies.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
