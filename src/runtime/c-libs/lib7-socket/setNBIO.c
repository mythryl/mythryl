/* setNBIO.c	-- "NBIO" == "non-blocking I/O"
 *
 * Set/clear nonblocking status on given socket.
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
#include "socket-util.h"



/*
###                 "This is the biggest fool thing we've ever
###                  done -- the bomb will never go off -- and
###                  I speak as an expert on explosives."
###
###                        -- Admiral William Leahy, 1945,
###                           speaking to President Truman about the atom bomb
*/



/* _lib7_Sock_setNBIO:  (Socket_Fd, Bool) -> Void
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t   _lib7_Sock_setNBIO   (lib7_state_t  *lib7_state,   lib7_val_t  arg)
{
    int	socket = REC_SELINT(arg, 0);
    int status;

#ifdef USE_FCNTL_FOR_NBIO
    int n = fcntl(F_GETFL, socket);

    if (n < 0)   return RAISE_SYSERR (lib7_state, n);

    if (REC_SEL(arg, 1) == LIB7_true)	n |=  O_NONBLOCK;
    else				n &= ~O_NONBLOCK;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        status = fcntl(F_SETFL, socket, n);

/*  } while (status < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

#else
    int n = (REC_SEL(arg, 1) == LIB7_true);

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        status = ioctl (socket, FIONBIO, (char *)&n);

/*  } while (status < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/
#endif

    CHECK_RETURN_UNIT(lib7_state, status);
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
