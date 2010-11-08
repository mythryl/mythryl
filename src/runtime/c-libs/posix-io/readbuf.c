/* readbuf.c
 *
 */

#include "../../config.h"

#include <errno.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_IO_readbuf : (int * rw_unt8_vector.Rw_Vector * int * int) -> int
 *                     fd    data               nbytes start
 *
 * Read nbytes of data from the specified file into the given array, 
 * starting at start. Return the number of bytes read. Assume bounds
 * have been checked.
 */
lib7_val_t _lib7_P_IO_readbuf (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		fd = REC_SELINT(arg, 0);
    lib7_val_t	buf = REC_SEL(arg, 1);
    int		nbytes = REC_SELINT(arg, 2);
    char	*start = STR_LIB7toC(buf) + REC_SELINT(arg, 3);
    int		n;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        n = read (fd, start, nbytes);

/*  } while (n < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    CHECK_RETURN (lib7_state, n)
}



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
