/* openf.c
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

/* _lib7_P_FileSys_openf : (String * word * word) -> int
 *                        name     flags  mode
 *
 * Open a file and return the file descriptor.
 */
lib7_val_t _lib7_P_FileSys_openf (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	    path  = REC_SEL(    arg, 0);
    int		    flags = REC_SELWORD(arg, 1);
    int		    mode  = REC_SELWORD(arg, 2);

    int		    fd;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        fd    = open (STR_LIB7toC(path), flags, mode);

/*  } while (fd < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    CHECK_RETURN(lib7_state, fd)
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
