/* chown.c
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

/* _lib7_P_FileSys_chown : (String * word * word) -> Void
 *                        name     uid    gid
 *
 * Change owner and group of file given its name.
 */
lib7_val_t _lib7_P_FileSys_chown (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	    path = REC_SEL(arg, 0);
    uid_t           uid = REC_SELWORD(arg, 1);
    gid_t           gid = REC_SELWORD(arg, 2);
    int		    status;

    status = chown (STR_LIB7toC(path), uid, gid);

    CHECK_RETURN_UNIT(lib7_state, status)

} /* end of _lib7_P_FileSys_chown */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
