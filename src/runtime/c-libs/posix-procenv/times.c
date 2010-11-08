/* times.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include <sys/times.h>
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_ProcEnv_times: Void -> int * int * int * int * int
 *
 * Return process and child process times, in clock ticks.
 */
lib7_val_t _lib7_P_ProcEnv_times (lib7_state_t *lib7_state, lib7_val_t arg)
{
    clock_t      t;
    struct tms   ts;
    lib7_val_t     v, e, u, s, cu, cs;

    t = times (&ts);

    if (t == -1)
        return RAISE_SYSERR(lib7_state, -1);

    INT32_ALLOC(lib7_state, e, t);
    INT32_ALLOC(lib7_state, u, ts.tms_utime);
    INT32_ALLOC(lib7_state, s, ts.tms_stime);
    INT32_ALLOC(lib7_state, cu, ts.tms_cutime);
    INT32_ALLOC(lib7_state, cs, ts.tms_cstime);
    REC_ALLOC5(lib7_state, v, e, u, s, cu, cs);

    return v;

} /* end of _lib7_P_ProcEnv_times */

/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
