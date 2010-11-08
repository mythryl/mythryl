/* listen.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/*
###      "A good listener is not only popular everywhere,
###       but after a while, knows something."
###
###                       -- Wilson Mizner
 */

/* _lib7_Sock_listen : (socket * int) -> Void
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_listen (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		socket = REC_SELINT(arg, 0);
    int		backlog = REC_SELINT(arg, 1);
    int		status;

    status = listen (socket, backlog);

    CHECK_RETURN_UNIT(lib7_state, status);

} /* end of _lib7_Sock_listen */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
