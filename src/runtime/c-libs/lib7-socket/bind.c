/* bind.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_Sock_bind : (socket * addr) -> Void
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t 

_lib7_Sock_bind (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		socket = REC_SELINT(arg, 0);
    lib7_val_t	addr = REC_SEL(arg, 1);
    int		status;

    status = bind (
	socket,
	GET_SEQ_DATAPTR(struct sockaddr, addr),
	GET_SEQ_LEN(addr));

    CHECK_RETURN_UNIT(lib7_state, status);
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
