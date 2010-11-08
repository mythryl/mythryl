/* symlink.c
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


/* _lib7_P_FileSys_symlink : String * String -> Void
 *                         existing newname
 *
 * Creates a symbolic link from newname to existing file.
 */
lib7_val_t _lib7_P_FileSys_symlink (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int		status;

    lib7_val_t	existing = REC_SEL(arg, 0);
    lib7_val_t	newname  = REC_SEL(arg, 1);

    status = symlink(STR_LIB7toC(existing), STR_LIB7toC(newname));

    CHECK_RETURN_UNIT (lib7_state, status)
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
