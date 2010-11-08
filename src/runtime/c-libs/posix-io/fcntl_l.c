/* fcntl_l.c
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

/* _lib7_P_IO_fcntl_l : int * int * flock_rep -> flock_rep
 *    flock_rep = int * int * offset * offset * int
 *
 * Handle record locking.
 */
lib7_val_t _lib7_P_IO_fcntl_l (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int              fd = REC_SELINT(arg, 0);
    int              cmd = REC_SELINT(arg, 1);
    lib7_val_t         flock_rep = REC_SEL(arg, 2), chunk;
    struct flock     flock;
    int              status;
    
    flock.l_type = REC_SELINT(flock_rep, 0);
    flock.l_whence = REC_SELINT(flock_rep, 1);
    flock.l_start = REC_SELINT(flock_rep, 2);
    flock.l_len = REC_SELINT(flock_rep, 3);
   
/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        status = fcntl(fd, cmd, &flock);

/*  } while (status < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    if (status < 0)
        return RAISE_SYSERR(lib7_state, status);

    REC_ALLOC5(lib7_state, chunk,
	INT_CtoLib7(flock.l_type),
	INT_CtoLib7(flock.l_whence), 
	INT_CtoLib7(flock.l_start),
	INT_CtoLib7(flock.l_len),
	INT_CtoLib7(flock.l_pid));

    return chunk;

} /* end of _lib7_P_IO_fcntl_l */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
