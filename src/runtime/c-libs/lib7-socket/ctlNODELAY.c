/* ctlNODELAY.c
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
#include "socket-util.h"

/* _lib7_Sock_ctlNODELAY : (socket * Bool option) -> Bool
 *
 * NOTE: this is a TCP level option, so we cannot use the utility function.
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/internet-socket.pkg
 */
lib7_val_t _lib7_Sock_ctlNODELAY (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		socket = REC_SELINT(arg, 0);
    lib7_val_t	ctl = REC_SEL(arg, 1);
    bool_t	flag;
    int		status;

    if (ctl == OPTION_NONE) {
	int	optSz = sizeof(int);
	status = getsockopt (socket, IPPROTO_TCP, TCP_NODELAY, (sockoptval_t)&flag, &optSz);
	ASSERT((status < 0) || (optSz == sizeof(int)));
    }
    else {
	flag = (bool_t)INT_LIB7toC(OPTION_get(ctl));
	status = setsockopt (socket, IPPROTO_TCP, TCP_NODELAY, (sockoptval_t)&flag, sizeof(int));
    }

    if (status < 0)
        return RAISE_SYSERR(lib7_state, status);
    else
	return (flag ? LIB7_true : LIB7_false);

} /* end of _lib7_Sock_ctlNODELAY */



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
