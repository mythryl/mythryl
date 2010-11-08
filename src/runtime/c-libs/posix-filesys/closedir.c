/* closedir.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"
#include "runtime-base.h"
#include "runtime-values.h"
#include "tags.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

/* _lib7_P_FileSys_closedir : chunk -> Void
 *
 * Close a directory stream.
 */
lib7_val_t _lib7_P_FileSys_closedir (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int        status;
    
    status = closedir(PTR_LIB7toC(DIR, arg));

    CHECK_RETURN_UNIT(lib7_state,status)

} /* end of _lib7_P_FileSys_closedir */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
