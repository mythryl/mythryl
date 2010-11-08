/* alarm.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

/* _lib7_P_Process_alarm : int -> int
 *
 * Set a process alarm clock
 */
lib7_val_t _lib7_P_Process_alarm (lib7_state_t *lib7_state, lib7_val_t arg)
{
    return INT_CtoLib7( alarm( INT_LIB7toC( arg )));
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
