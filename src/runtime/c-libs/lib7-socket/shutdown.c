/* shutdown.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_Sock_shutdown : (socket * int) -> Void
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_shutdown (lib7_state_t *lib7_state, lib7_val_t arg)
{
    if (shutdown (REC_SELINT(arg, 0), REC_SELINT(arg, 1)) < 0)
        return RAISE_SYSERR(lib7_state, status);
    else
	return LIB7_void;

} /* end of _lib7_Sock_shutdown */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
