/* sendbufto.c
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
###                         "The Americans are good about making fancy cars and refrigerators,
###                          but that doesn't mean they are any good at making aircraft.
###                          They are bluffing. They are excellent at bluffing."
###
###                                   -- Hermann Goering, Commander-in-Chief of the Luftwaffe, 1942
 */



/* _lib7_Sock_sendbufto : (socket * bytes * int * int * Bool * Bool * addr) -> int
 *
 * Send data from the buffer; bytes is either
 * a rw_unt8_vector::Rw_Vector,
 * or a unt8_vector::Vector.
 *
 * The arguments are:
 *     socket
 *     data buffer
 *     start position
 *     number of bytes
 *     OOB flag
 *     don't_route flag
 *     destination address
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_sendbufto (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		socket   =  REC_SELINT(arg, 0);
    lib7_val_t	buf    =  REC_SEL(   arg, 1);
    int		nbytes =  REC_SELINT(arg, 3);
    char*	data   =  STR_LIB7toC(buf) + REC_SELINT(arg, 2);
    lib7_val_t	addr   =  REC_SEL(arg, 6);

    /* Compute flags parameter: */
    int flgs = 0;
    if (REC_SEL(arg, 4) == LIB7_true) flgs |= MSG_OOB;
    if (REC_SEL(arg, 5) == LIB7_true) flgs |= MSG_DONTROUTE;

    {   int n;

/*      do { */		/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

	    n = sendto (
		    socket,
		    data,
		    nbytes,
		    flgs,
		    GET_SEQ_DATAPTR (struct sockaddr, addr),
		    GET_SEQ_LEN(addr)
		);

/*      } while (n < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

        CHECK_RETURN (lib7_state, n);
    }

} /* end of _lib7_Sock_sendbufto */


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
