/* setsid.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_ProcEnv_setsid: Void -> int
 *
 * Set session id
 */
lib7_val_t _lib7_P_ProcEnv_setsid (lib7_state_t *lib7_state, lib7_val_t arg)
{
    pid_t      pid;

    pid = setsid ();

    CHECK_RETURN(lib7_state, pid)
}



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
