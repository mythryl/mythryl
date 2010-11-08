/* fcntl_l_64.c
 *
 *   Using 64-bit position values represented as 32-bit pairs.
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

/* _lib7_P_IO_fcntl_l_64 : int * int * flock_rep -> flock_rep
 *    flock_rep = int * int * offsethi * offsetlo * offsethi * offsetlo * int
 *
 * Handle record locking.
 */
lib7_val_t _lib7_P_IO_fcntl_l_64 (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int              fd = REC_SELINT(arg, 0);
    int              cmd = REC_SELINT(arg, 1);
    lib7_val_t         flock_rep = REC_SEL(arg, 2), chunk;
    lib7_val_t         starthi, startlo, lenhi, lenlo;
    struct flock     flock;
    int              status;
    
    flock.l_type   = REC_SELINT(flock_rep, 0);
    flock.l_whence = REC_SELINT(flock_rep, 1);

    if (sizeof(flock.l_start) > 4)
      flock.l_start =
	(((off_t)WORD_LIB7toC(REC_SEL(flock_rep, 2))) << 32) |
	((off_t)(WORD_LIB7toC(REC_SEL(flock_rep, 3))));
    else
      flock.l_start =
	(off_t)(WORD_LIB7toC(REC_SEL(flock_rep, 3)));

    if (sizeof (flock.l_len) > 4)
      flock.l_len =
	(((off_t)WORD_LIB7toC(REC_SEL(flock_rep, 4))) << 32) |
	((off_t)(WORD_LIB7toC(REC_SEL(flock_rep, 5))));
    else
      flock.l_len =
	(off_t)(WORD_LIB7toC(REC_SEL(flock_rep, 5)));
   
/*  do { */	/* Backed out 2010-02-26 CrT: See discussion at bottom of src/runtime/c-libs/lib7-socket/connect.c	*/

        status = fcntl(fd, cmd, &flock);

/*  } while (status < 0 && errno == EINTR);	*/	/* Restart if interrupted by a SIGALRM or SIGCHLD or whatever.	*/

    if (status < 0)
        return RAISE_SYSERR(lib7_state, status);

    if (sizeof(flock.l_start) > 4) {
      WORD_ALLOC (lib7_state, starthi, (Unsigned32_t) (flock.l_start >> 32));
    } else {
      WORD_ALLOC (lib7_state, starthi, (Unsigned32_t) 0);
    }
    WORD_ALLOC (lib7_state, startlo, (Unsigned32_t) flock.l_start);

    if (sizeof(flock.l_len) > 4) {
      WORD_ALLOC (lib7_state, lenhi, (Unsigned32_t) (flock.l_len >> 32));
    } else {
      WORD_ALLOC (lib7_state, lenhi, (Unsigned32_t) 0);
    }

    WORD_ALLOC (lib7_state, lenlo, (Unsigned32_t) flock.l_len);

    LIB7_AllocWrite (lib7_state, 0, MAKE_DESC (DTAG_record, 7));
    LIB7_AllocWrite (lib7_state, 1, INT_CtoLib7(flock.l_type));
    LIB7_AllocWrite (lib7_state, 2, INT_CtoLib7(flock.l_whence));
    LIB7_AllocWrite (lib7_state, 3, starthi);
    LIB7_AllocWrite (lib7_state, 4, startlo);
    LIB7_AllocWrite (lib7_state, 5, lenhi);
    LIB7_AllocWrite (lib7_state, 6, lenlo);
    LIB7_AllocWrite (lib7_state, 7, INT_CtoLib7(flock.l_pid));
    chunk = LIB7_Alloc (lib7_state, 7);

    return chunk;

} /* end of _lib7_P_IO_fcntl_l_64 */


/* Copyright (c) 2004 by The Fellowship of SML/NJ
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
