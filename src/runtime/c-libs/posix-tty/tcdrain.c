/* tcdrain.c
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

/* _lib7_P_TTY_tcdrain : int -> Void
 *
 * Wait for all output to be transmitted.
 */
lib7_val_t _lib7_P_TTY_tcdrain (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int fd = INT_LIB7toC(arg);

    int status = tcdrain(fd);

    CHECK_RETURN_UNIT(lib7_state, status)
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
