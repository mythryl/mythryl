/* ctlDONTROUTE.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"
#include "socket-util.h"

/*
###        "The popular mind often pictures gigantic flying machines
###         speeding across the Atlantic carrying innumerable passengers
###         in a way analogous to our modern steamships.
###         It seems safe to say that such ideas are wholly visionary." 
###
###                    -- William H. Pickering, astronomer 1910
 */

/* _lib7_Sock_ctlDONTROUTE : (Socket, Null_Or( Bool )) -> Bool
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t

_lib7_Sock_ctlDONTROUTE (lib7_state_t *lib7_state, lib7_val_t arg)
{
    return _util_Sock_ControlFlg (lib7_state, arg, SO_DONTROUTE);
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
