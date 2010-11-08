/* accept.c
 *
 */

#include "../../config.h"

#include <errno.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_Sock_accept : Socket -> (Socket, Address)
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_accept (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		socket = INT_LIB7toC(arg);
    char	addrBuf[MAX_SOCK_ADDR_SZB];
    int		addrLen = MAX_SOCK_ADDR_SZB;
    int		newSock;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        newSock = accept (socket, (struct sockaddr *)addrBuf, &addrLen);

/*  } while (newSock < 0 && errno == EINTR);	*/		/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    if (newSock == -1) {
        return RAISE_SYSERR(lib7_state, newSock);
    } else {
	lib7_val_t	data = LIB7_CData (lib7_state, addrBuf, addrLen);
	lib7_val_t	addr, res;

	SEQHDR_ALLOC(lib7_state, addr, DESC_word8vec, data, addrLen);
	REC_ALLOC2(lib7_state, res, INT_CtoLib7(newSock), addr);
	return res;
    }
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
