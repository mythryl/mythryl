/* sendbuf.c
 *
 */

#include "../../config.h"

#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#include "print-if.h"
#include "hexdump-if.h"


/*
###         "Railroad carriages are pulled at the enormous speed
###          of fifteen miles per hour by engines which,
###          in addition to endangering life and limb of passengers,
###          roar and snort their way through the countryside,
###          setting fire to the crops, scaring the livestock,
###          and frightening women and children.
###
###         "The Almighty certainly never intended that
###          people should travel at such break-neck speed."
### 
###                -- President Martin Van Buren, 1829
 */



/* _lib7_Sock_sendbuf
 *     :
 *     ( Int,		# socket fd
 *       Wy8Vector,     # byte vector
 *       Int,           # start offset
 *       Int,           # vector length (end offset)
 *       Bool,          # don't-route flag
 *       Bool           # default-oob flag
 *     )
 *     ->
 *     Int
 *
 * Send data from the buffer; bytes is either a rw_unt8_vector.Rw_Vector, or
 * a unt8_vector.vector.  The arguments are: socket, data buffer, start
 * position, number of bytes, OOB flag, and don't_route flag.
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_sendbuf (lib7_state_t *lib7_state, lib7_val_t arg)
{

    int		   socket    = REC_SELINT(                   arg, 0);
    lib7_val_t	   buf       = REC_SEL(                      arg, 1);
    unsigned char* data      = STR_LIB7toC(buf) + REC_SELINT(arg, 2);
    int		   nbytes    = REC_SELINT(                   arg, 3);
    lib7_val_t     oob       = REC_SEL(                      arg, 4);
    lib7_val_t     dontroute = REC_SEL(                      arg, 5);

    /* Compute flags parameter:
    */
    int flgs = 0;
    if (oob       == LIB7_true) flgs |= MSG_OOB;
    if (dontroute == LIB7_true) flgs |= MSG_DONTROUTE;

    print_if(   "sendbuf.c/top: socket d=%d nbytes d=%d OOB=%s DONTROUTE=%s\n", socket, nbytes, (oob == LIB7_true) ? "TRUE" : "FALSE", (dontroute == LIB7_true) ? "TRUE" : "FALSE" );
    hexdump_if( "sendbuf.c/top: Data to send: ", data, nbytes );

    errno = 0;

    {   int n;

/*      do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

            n = send (socket, data, nbytes, flgs);

/*      } while (n < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

        print_if( "sendbuf.c/bot: n d=%d errno d=%d\n", n, errno );

        CHECK_RETURN (lib7_state, n);
    }
}


/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
