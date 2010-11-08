/* ctlDEBUG.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "socket-util.h"
#include "cfun-proto-list.h"

/* _lib7_Sock_ctlDEBUG : (Socket, Null_Or( Bool )) -> Bool
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t

_lib7_Sock_ctlDEBUG (lib7_state_t *lib7_state, lib7_val_t arg)
{
    return _util_Sock_ControlFlg (lib7_state, arg, SO_DEBUG);
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
