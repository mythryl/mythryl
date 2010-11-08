/* mkfifo.c
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
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"


/* _lib7_P_FileSys_mkfifo : (String * word) -> Void
 *                         name     mode
 *
 * Make a FIFO special file.
 */
lib7_val_t _lib7_P_FileSys_mkfifo (lib7_state_t *lib7_state, lib7_val_t arg)
{
    lib7_val_t	    path = REC_SEL(arg, 0);
    mode_t	    mode = REC_SELWORD(arg, 1);
    int		    status;

    status = mkfifo (STR_LIB7toC(path), mode);

    CHECK_RETURN_UNIT(lib7_state, status)

} /* end of _lib7_P_FileSys_mkfifo */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
