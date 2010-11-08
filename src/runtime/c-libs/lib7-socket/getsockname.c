/* getsockname.c
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



/*
###              "The radio craze will die out in time."
###                               -- Thomas Edison, 1922
*/



/* _lib7_Sock_getsockname : socket -> addr
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_getsockname (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		socket = INT_LIB7toC(arg);
    char	addrBuf[MAX_SOCK_ADDR_SZB];
    int		addrLen = MAX_SOCK_ADDR_SZB;
    int		status;

    status = getsockname (socket, (struct sockaddr *)addrBuf, &addrLen);

    if (status == -1) {

        return RAISE_SYSERR(lib7_state, status);

    } else {
	lib7_val_t	data = LIB7_CData (lib7_state, addrBuf, addrLen);
	lib7_val_t	addr;
	SEQHDR_ALLOC (lib7_state, addr, DESC_word8vec, data, addrLen);
	return addr;
    }

} /* end of _lib7_Sock_getsockname */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
