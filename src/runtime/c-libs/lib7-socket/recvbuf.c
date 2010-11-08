/* recvbuf.c
 *
 */

#include "../../config.h"

#include <errno.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/*
###                   "Before man reaches the moon, mail will be
###                    delivered within hours from New York to
###                    California, to Britain, to India or Australia
###                    by guided missiles.
###                       We stand on the threshold of rocket mail."
###
###                          -- Arthur Summerfield, US Postmaster General, 1959 
 */

/* _lib7_Sock_recvbuf : (socket * rw_unt8_vector.Rw_Vector * int * int * Bool * Bool) -> int
 *
 * The arguments are: socket, data buffer, start position, number of
 * bytes, OOB flag and peek flag.
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_recvbuf (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		socket = REC_SELINT(                   arg, 0);
    lib7_val_t	buf    = REC_SEL(                      arg, 1);
    char	*start = STR_LIB7toC(buf) + REC_SELINT(arg, 2);
    int		nbytes = REC_SELINT(                   arg, 3);
    int		flag = 0;
    int		n;

    if (REC_SEL(arg, 4) == LIB7_true) flag |= MSG_OOB;
    if (REC_SEL(arg, 5) == LIB7_true) flag |= MSG_PEEK;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        n = recv (socket, start, nbytes, flag);

/*  } while (n < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/


    CHECK_RETURN (lib7_state, n)

} /* end of _lib7_Sock_recvbuf */



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
