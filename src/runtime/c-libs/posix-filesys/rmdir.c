/* rmdir.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"


/* _lib7_P_FileSys_rmdir : String -> Void
 *
 * Remove a directory
 */
lib7_val_t _lib7_P_FileSys_rmdir (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		status;

    status = rmdir(STR_LIB7toC(arg));

    CHECK_RETURN_UNIT(lib7_state, status)

} /* end of _lib7_P_FileSys_rmdir */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
