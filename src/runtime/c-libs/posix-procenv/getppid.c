/* getppid.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

/* _lib7_P_ProcEnv_getppid : Void -> int
 *
 * Return the process id of the parent process.
 */
lib7_val_t _lib7_P_ProcEnv_getppid (lib7_state_t *lib7_state, lib7_val_t arg)
{
    return INT_CtoLib7(getppid());

} /* end of _lib7_P_ProcEnv_getppid */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
