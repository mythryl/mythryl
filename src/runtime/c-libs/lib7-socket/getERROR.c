/* getERROR.c
 *
 */

#include "../../config.h"

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_Sock_getERROR : socket -> Bool
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_getERROR (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		socket = INT_LIB7toC(arg);
    int		flag, status, optSz = sizeof(int);

    status = getsockopt (socket, SOL_SOCKET, SO_ERROR, (sockoptval_t)&flag, &optSz);

    if (status < 0)
        return RAISE_SYSERR(lib7_state, status);
    else
	return (flag ? LIB7_true : LIB7_false);

} /* end of _lib7_Sock_getERROR */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
