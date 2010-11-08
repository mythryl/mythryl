/* fchown.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

/* _lib7_P_FileSys_fchown : (int * word * word) -> Void
 *                         fd     uid    gid
 *
 * Change owner and group of file given a file descriptor for it.
 */
lib7_val_t _lib7_P_FileSys_fchown (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int	            fd =  REC_SELINT (arg, 0);
    uid_t           uid = REC_SELWORD(arg, 1);
    gid_t           gid = REC_SELWORD(arg, 2);
    int		    status;

    status = fchown (fd, uid, gid);

    CHECK_RETURN_UNIT(lib7_state, status)

} /* end of _lib7_P_FileSys_fchown */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
