/* getTYPE.c
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

/* _lib7_Sock_getTYPE : socket -> sock_type
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_getTYPE (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		socket = INT_LIB7toC(arg);
    int		flag, status, optSz = sizeof(int);

    status = getsockopt (socket, SOL_SOCKET, SO_TYPE, (sockoptval_t)&flag, &optSz);

    if (status < 0)
        return RAISE_SYSERR(lib7_state, status);
    else
	return LIB7_SysConst (lib7_state, &_Sock_Type, flag);

} /* end of _lib7_Sock_getTYPE */



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
