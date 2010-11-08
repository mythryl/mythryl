/* recv.c
 *
 */

#include "../../config.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "sockets-osdep.h"
#include INCLUDE_SOCKET_H
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#include "print-if.h"
#include "hexdump-if.h"

/* _lib7_Sock_recv : (Socket, Int, Bool, Bool) -> unt8_vector::Vector
 *
 * The arguments are: socket, number of bytes, OOB flag and peek flag; the
 * result is the vector of bytes received.
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_recv (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	vec;
    lib7_val_t	result;
    int		n;

    int		socket = REC_SELINT(arg, 0);
    int		nbytes = REC_SELINT(arg, 1);
    lib7_val_t	oob    = REC_SEL(   arg, 2);
    lib7_val_t	peek   = REC_SEL(   arg, 3);

    int		flag = 0;

    if (oob  == LIB7_true) flag |= MSG_OOB;
    if (peek == LIB7_true) flag |= MSG_PEEK;

    /* Allocate the vector.
     * Note that this might cause a GC:
     */
    vec = LIB7_AllocRaw32 (lib7_state, BYTES_TO_WORDS(nbytes));

    print_if("recv.c/before: socket d=%d nbytes d=%d oob=%s peek=%s\n",socket,nbytes,(oob == LIB7_true)?"TRUE":"FALSE",(peek == LIB7_true)?"TRUE":"FALSE");
    errno = 0;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        n = recv (socket, PTR_LIB7toC(char, vec), nbytes, flag);

/*  } while (n < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    print_if(   "recv.c/after: n d=%d errno d=%d (%s)\n", n, errno, errno ? strerror(errno) : "");
    hexdump_if( "recv.c/after: Received data: ", PTR_LIB7toC(unsigned char, vec), n );

    if (n < 0)
        return RAISE_SYSERR(lib7_state, status);
    else if (n == 0)
	return LIB7_string0;

    if (n < nbytes) {
      /* we need to shrink the vector */
	LIB7_ShrinkRaw32 (lib7_state, vec, BYTES_TO_WORDS(n));
    }

    SEQHDR_ALLOC (lib7_state, result, DESC_string, vec, n);

    return result;
}



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
