/* fcntl_d.c
 *
 */

#include "../../config.h"

#include <errno.h>

#include "runtime-unixdep.h"

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_IO_fcntl_d : int * int -> int
 *
 * Duplicate an open file descriptor
 */
lib7_val_t _lib7_P_IO_fcntl_d (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int             fd;
    int             fd0 = REC_SELINT(arg, 0);
    int             fd1 = REC_SELINT(arg, 1);

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        fd = fcntl(fd0, F_DUPFD, fd1);

/*  } while (fd < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    CHECK_RETURN(lib7_state, fd)

} /* end of _lib7_P_IO_fcntl_d */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
