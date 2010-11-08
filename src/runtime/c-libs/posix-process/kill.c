/* kill.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include <signal.h>

/*
###        "From too much love of living,
###         From hope and fear set free,
###         We thank with brief thanksgiving,
###         Whatever gods may be,
###         That no life lives forever,
###         That dead men rise up never,
###         That even the weariest river
###         Winds somewhere safe to sea."
###
###                     -- Swinburne
 */


/* _lib7_P_Process_kill : (Int, Int) -> Void
 *
 * Send a signal to a process or a group of processes
 */
lib7_val_t _lib7_P_Process_kill (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int status = kill(REC_SELINT(arg, 0),REC_SELINT(arg, 1));

    CHECK_RETURN_UNIT (lib7_state, status)
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
