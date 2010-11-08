/* dup.c
 *
 */

#include "../../config.h"

#include <errno.h>

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

/* _lib7_P_IO_dup : int -> int
 *
 * Duplicate an open file descriptor
 */
lib7_val_t _lib7_P_IO_dup (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int             fd0 = INT_LIB7toC(arg);
    int             fd1;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        fd1 = dup(fd0);

/*  } while (fd1 < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    CHECK_RETURN(lib7_state, fd1)

} /* end of _lib7_P_IO_dup */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
