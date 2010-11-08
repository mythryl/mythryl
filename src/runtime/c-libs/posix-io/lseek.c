/* lseek.c
 *
 */

#include "../../config.h"

#include "runtime-unixdep.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

/* _lib7_P_IO_lseek : int * int * int -> int
 *
 * Move read/write file pointer.
 */
lib7_val_t _lib7_P_IO_lseek (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int         fd = REC_SELINT(arg, 0);
    off_t       offset = REC_SELINT(arg, 1), pos;
    int         whence = REC_SELINT(arg, 2);

    pos = lseek(fd, offset, whence);

    CHECK_RETURN(lib7_state, pos)

} /* end of _lib7_P_IO_lseek */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
