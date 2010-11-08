/* time.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include <time.h>

/* _lib7_P_ProcEnv_time: Void -> int32.Int
 *
 * Return time in seconds from 00:00:00 UTC, January 1, 1970
 */
lib7_val_t _lib7_P_ProcEnv_time (lib7_state_t *lib7_state, lib7_val_t arg)
{
    time_t      t;
    lib7_val_t	result;

    t = time (NULL);

    INT32_ALLOC(lib7_state, result, t);

    return result;
}



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
