/* read.c
 */

#include "../../config.h"

#include <errno.h>

#include "runtime-unixdep.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"



/*
###		"Read at every wait; read at all hours;
###              read within leisure; read in times of labor;
###              read as one goes in; read as one goes out.
###              The task of an educated mind is simply put:
###                 read to lead."
###
###                                      -- Cicero
*/



/* _lib7_P_IO_read : (int * int) -> unt8_vector.Vector
 *                  fd    nbytes
 *
 * Read the specified number of bytes from the specified file,
 * returning them in a vector.
 */
lib7_val_t _lib7_P_IO_read (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int fd     = REC_SELINT(arg, 0);
    int nbytes = REC_SELINT(arg, 1);

    if (nbytes == 0)
	return LIB7_string0;

    /* Allocate the vector.
     * Note that this might cause a GC:
     */
    {   lib7_val_t vec = LIB7_AllocRaw32 (lib7_state, BYTES_TO_WORDS(nbytes));

        int n;

/*      do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

            n = read (fd, PTR_LIB7toC(char, vec), nbytes);

/*      } while (n < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

	if (n < 0)
	    return RAISE_SYSERR(lib7_state, n);
	else if (n == 0)
	    return LIB7_string0;

	if (n < nbytes) {
	    /* We need to shrink the vector */
	    LIB7_ShrinkRaw32 (lib7_state, vec, BYTES_TO_WORDS(n));
	}

        {   lib7_val_t result;
	    SEQHDR_ALLOC (lib7_state, result, DESC_string, vec, n);
	    return result;
	}
    }
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
