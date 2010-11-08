/* socketpair.c
 *
 * NOTE: this file is UNIX specific.
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include "sockets-osdep.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



/*
###                  "Computers in the future may weigh no more than 1.5 tons."
###
###                                            -- Popular Mechanics, 1949
*/



/* _lib7_Sock_socketpair : (int * int * int) -> (socket * socket)
 *
 * Create a pair of sockets.  The arguments are: domain (should be
 * AF_UNIX), type, and protocol.
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/generic-socket.pkg

 */
lib7_val_t _lib7_Sock_socketpair (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int	 domain   = REC_SELINT(arg, 0);
    int	 type     = REC_SELINT(arg, 1);
    int	 protocol = REC_SELINT(arg, 2);

    int	 status;
    int	 socket[2];

    status = socketpair (domain, type, protocol, socket);

    if (status < 0) {

        return RAISE_SYSERR(lib7_state, status);

    } else {

	lib7_val_t	res;
	REC_ALLOC2(lib7_state, res, INT_CtoLib7(socket[0]), INT_CtoLib7(socket[1]));
	return res;
    }

} /* end of _lib7_Sock_socketpair */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */

