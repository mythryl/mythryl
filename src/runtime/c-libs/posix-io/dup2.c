/* dup2.c
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

/* _lib7_P_IO_dup2 : int * int -> Void
 *
 * Duplicate an open file descriptor
 */
lib7_val_t _lib7_P_IO_dup2 (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int             status;
    int             fd0 = REC_SELINT(arg, 0);
    int             fd1 = REC_SELINT(arg, 1);

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        status = dup2(fd0, fd1);

/*  } while (status < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    CHECK_RETURN_UNIT(lib7_state,status)
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
