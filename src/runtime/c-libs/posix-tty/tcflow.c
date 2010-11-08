/* tcflow.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_TTY_tcflow : (Int, Int) -> Void
 *
 * Suspend transmission or receipt of data.
 */
lib7_val_t _lib7_P_TTY_tcflow (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int status = tcflow(REC_SELINT(arg, 0),REC_SELINT(arg, 1));

    CHECK_RETURN_UNIT(lib7_state, status)
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
