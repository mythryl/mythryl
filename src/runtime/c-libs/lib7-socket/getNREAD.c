/* getNREAD.c
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

/* _lib7_Sock_getNREAD : socket -> int
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_getNREAD (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		n, status;

    status = ioctl (INT_LIB7toC(arg), FIONREAD, (char *)&n);

    if (status < 0)
        return RAISE_SYSERR(lib7_state, status);
    else
	return INT_CtoLib7(n);

} /* end of _lib7_Sock_getNREAD */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
