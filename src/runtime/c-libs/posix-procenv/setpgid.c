/* setpgid.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

/* _lib7_P_ProcEnv_setpgid: int * int -> Void
 *
 * Set user id
 */
lib7_val_t _lib7_P_ProcEnv_setpgid (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int         status;

    status = setpgid(REC_SELINT(arg, 0),REC_SELINT(arg, 1));

    CHECK_RETURN_UNIT(lib7_state, status)

} /* end of _lib7_P_ProcEnv_setpgid */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
