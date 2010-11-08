/* access.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "runtime-base.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"


/* _lib7_P_FileSys_access : (String * word) -> Bool
 *                         name     Access_Mode
 *
 * Determine accessibility of a file.
 */
lib7_val_t _lib7_P_FileSys_access (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	    path = REC_SEL(arg, 0);
    mode_t	    mode = REC_SELWORD(arg, 1);
    int		    status;

    status = access (STR_LIB7toC(path), mode);

    if (status == 0)
        return LIB7_true;
    else
        return LIB7_false;

} /* end of _lib7_P_FileSys_access */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
