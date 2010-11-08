/* ctermid.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include <stdio.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_ProcEnv_ctermid: Void -> String
 *
 * Return pathname of controlling terminal.
 */
lib7_val_t _lib7_P_ProcEnv_ctermid (lib7_state_t *lib7_state, lib7_val_t arg)
{
    char     name[L_ctermid];
    char     *status;

    status = ctermid(name);
    if (status == NULL || *status == '\0')
      return RAISE_ERROR(lib7_state, "cannot determine controlling terminal");
  
    return LIB7_CString (lib7_state, name);

} /* end of _lib7_P_ProcEnv_ctermid */



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
