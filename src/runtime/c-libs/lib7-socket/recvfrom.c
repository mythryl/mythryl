/* recvfrom.c
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
###           "Heavier-than-air flying machines are impossible."
###                              -- Lord Kelvin, 1895
*/

/* _lib7_Sock_recvfrom : (socket * int * Bool * Bool) -> (unt8_vector.Vector * addr)
 *
 * The arguments are:
 *      socket,
 *      number of bytes,
 *      OOB flag
 *      peek flag
 *
 *  The result is the vector of bytes read,
 *  and the source address.
 *
 * This function gets imported into the Mythryl world via:
 *     src/lib/std/src/socket/socket-guts.pkg
 */
lib7_val_t _lib7_Sock_recvfrom (lib7_state_t *lib7_state, lib7_val_t arg)
{
    char addrBuf[MAX_SOCK_ADDR_SZB];

    int  addrLen = MAX_SOCK_ADDR_SZB;

    int  socket   = REC_SELINT(arg, 0);
    int  nbytes = REC_SELINT(arg, 1);

    int  flag = 0;

    if (REC_SEL(arg, 2) == LIB7_true) flag |= MSG_OOB;
    if (REC_SEL(arg, 3) == LIB7_true) flag |= MSG_PEEK;

    /* Allocate the vector.
     * Note that this might cause a GC:
     */
    {   lib7_val_t vec = LIB7_AllocRaw32 (lib7_state, BYTES_TO_WORDS(nbytes));

        int n;

/*      do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

            n = recvfrom (
	            socket,
                    PTR_LIB7toC (char, vec),
                    nbytes,
                    flag,
	            (struct sockaddr *)addrBuf,
                    &addrLen
                );

/*      } while (n < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

	if (n < 0) {
  	    return RAISE_SYSERR(lib7_state, status);
	} else {
	    lib7_val_t	data = LIB7_CData (lib7_state, addrBuf, addrLen);
	    lib7_val_t	addr;
	    lib7_val_t	result;

	    if (n == 0)
		result = LIB7_string0;
	    else {
	        if (n < nbytes) {
		    /* We need to shrink the vector: */
		    LIB7_ShrinkRaw32 (lib7_state, vec, BYTES_TO_WORDS(n));
                }
		SEQHDR_ALLOC (lib7_state, result, DESC_string, vec, n);
	    }

	    SEQHDR_ALLOC (lib7_state, addr, DESC_word8vec, data, addrLen);
	    REC_ALLOC2(lib7_state, result, result, addr);

	    return result;
	}
    }
}



/* COPYRIGHT (c) 1995 AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
