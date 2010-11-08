/* fcntl_gfl.c
 *
 */

#include "../../config.h"

#include <errno.h>

#include "runtime-unixdep.h"

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_IO_fcntl_gfl : int -> word * word
 *
 * Get file status flags and file access modes.
 */
lib7_val_t _lib7_P_IO_fcntl_gfl (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int             fd = INT_LIB7toC(arg);
    int             flag;
    lib7_val_t      flags;
    lib7_val_t      mode;
    lib7_val_t      chunk;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        flag = fcntl(fd, F_GETFD);

/*  } while (flag < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    if (flag < 0)
        return RAISE_SYSERR(lib7_state, flag);

    WORD_ALLOC (lib7_state, flags, (flag & (~O_ACCMODE)));
    WORD_ALLOC (lib7_state, mode, (flag & O_ACCMODE));
    REC_ALLOC2(lib7_state, chunk, flags, mode);

    return chunk;

} /* end of _lib7_P_IO_fcntl_gfl */



/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
