/* rename.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include <stdio.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_FileSys_rename : String * String -> Void
 *                        oldname  newname
 *
 * Change the name of a file
 */
lib7_val_t _lib7_P_FileSys_rename (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	oldname = REC_SEL(arg, 0);
    lib7_val_t	newname = REC_SEL(arg, 1);

    int status = rename(STR_LIB7toC(oldname), STR_LIB7toC(newname));

    CHECK_RETURN_UNIT (lib7_state, status)
}


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
