/* recvbuffrom.c
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

/*
###           "Never underestimate the bandwidth
###            of a station wagon full of tapes
###            hurtling down the highway."
###
###                        -- Andrew Tannenbaum
 */

/* _lib7_Sock_recvbuffrom
 *   : (socket * rw_unt8_vector.Rw_Vector * int * int * Bool * Bool) -> (int * addr)
 *
 * The arguments are: socket, data buffer, start position, number of
 * bytes, OOB flag and peek flag.  The result is number of bytes read and
 * the source address.
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_recvbuffrom (lib7_state_t *lib7_state, lib7_val_t arg)
{
    char	addrBuf[MAX_SOCK_ADDR_SZB];
    int		addrLen = MAX_SOCK_ADDR_SZB;
    int		socket = REC_SELINT(arg, 0);
    lib7_val_t	buf = REC_SEL(arg, 1);
    int		nbytes = REC_SELINT(arg, 3);
    char	*start = STR_LIB7toC(buf) + REC_SELINT(arg, 2);
    int		flag = 0;
    int		n;

    if (REC_SEL(arg, 4) == LIB7_true) flag |= MSG_OOB;
    if (REC_SEL(arg, 5) == LIB7_true) flag |= MSG_PEEK;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        n = recvfrom (socket, start, nbytes, flag, (struct sockaddr *)addrBuf, &addrLen);

/*  } while (n < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    if (n < 0)
        return RAISE_SYSERR(lib7_state, status);
    else {
	lib7_val_t	data = LIB7_CData (lib7_state, addrBuf, addrLen);
	lib7_val_t	addr, res;

	SEQHDR_ALLOC (lib7_state, addr, DESC_word8vec, data, addrLen);
	REC_ALLOC2(lib7_state, res, INT_CtoLib7(n), addr);
	return res;
    }

} /* end of _lib7_Sock_recvbuffrom */



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
