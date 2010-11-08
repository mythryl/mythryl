/* write.c
 *
 */

#include "../../config.h"

#include <errno.h>

#include "runtime-unixdep.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"


/* _lib7_P_IO_write : (int * unt8_vector.Vector * int) -> int
 *
 * Write the number of bytes of data from the given vector,
 * starting at index 0, to the specified file.  Return the
 * number of bytes written. Assume bounds checks have been done.
 */
lib7_val_t _lib7_P_IO_write (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		fd = REC_SELINT(arg, 0);
    lib7_val_t	data = REC_SEL(arg, 1);
    size_t	nbytes = REC_SELINT(arg, 2);
    ssize_t    	n;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        n = write (fd, STR_LIB7toC(data), nbytes);

/*  } while (n < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    CHECK_RETURN (lib7_state, n)
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
