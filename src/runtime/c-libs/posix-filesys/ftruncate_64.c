/* ftruncate_64.c
 *
 *   Version of ftruncate with 64-positions passed as pair of 32-bit values.
 *
 */

#include "../../config.h"

#include <errno.h>

#include "runtime-unixdep.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

/* _lib7_P_FileSys_ftruncate_64 : (int * word32 * word32) -> Void
 *                               fd   lengthhi  lengthlo
 *
 * Make a directory
 */
lib7_val_t _lib7_P_FileSys_ftruncate_64 (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		    fd = REC_SELINT(arg, 0);
    off_t	    len =
      (sizeof(off_t) > 4)
      ? (((off_t)WORD_LIB7toC(REC_SEL(arg, 1))) << 32) |
        ((off_t)(WORD_LIB7toC(REC_SEL(arg, 2))))
      : ((off_t)(WORD_LIB7toC(REC_SEL(arg, 2))));
    int		    status;

/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        status = ftruncate (fd, len);

/*  } while (status < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    CHECK_RETURN_UNIT(lib7_state, status)

} /* end of _lib7_P_FileSys_ftruncate_64 */


/* Copyright (c) 2004 by The Fellowship of SML/NJ
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
