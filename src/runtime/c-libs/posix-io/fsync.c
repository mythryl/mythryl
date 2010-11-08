/* fsync.c
 *
 */

#include "../../config.h"

#include "runtime-base.h"
#include "runtime-values.h"
#include "runtime-heap.h"
#include "lib7-c.h"
#include "cfun-proto-list.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

/* _lib7_P_IO_fsync : int -> Void
 *
 * Synchronize  a  file's in-core state with storage
 */
lib7_val_t _lib7_P_IO_fsync (lib7_state_t *lib7_state, lib7_val_t arg)
{
    int fd = INT_LIB7toC(arg);

    int status = fsync(fd);

    CHECK_RETURN_UNIT(lib7_state, status)

} /* end of _lib7_P_IO_fsync */


/* COPYRIGHT (c) 1995 by AT&T Bell Laboratories.
 * Subsequent changes by Jeff Prothero Copyright (c) 2010,
 * released under Gnu Public Licence version 3.
 */
