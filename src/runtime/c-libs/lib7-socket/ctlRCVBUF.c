/* ctlRCVBUF.c
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

/* _lib7_Sock_ctlRCVBUF : (socket * int option) -> int
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_ctlRCVBUF (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		socket = REC_SELINT(arg, 0);
    lib7_val_t	ctl = REC_SEL(arg, 1);
    int		size, status;

    if (ctl == OPTION_NONE) {
	int	optSz = sizeof(int);
	status = getsockopt (socket, SOL_SOCKET, SO_RCVBUF, (sockoptval_t)&size, &optSz);
	ASSERT((status < 0) || (optSz == sizeof(int)));
    }
    else {
	size = INT_LIB7toC(OPTION_get(ctl));
	status = setsockopt (socket, SOL_SOCKET, SO_RCVBUF, (sockoptval_t)&size, sizeof(int));
    }

    if (status < 0)
        return RAISE_SYSERR(lib7_state, status);
    else
	return INT_CtoLib7(size);

} /* end of _lib7_Sock_ctlRCVBUF */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
